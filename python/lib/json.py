#!/usr/bin/env python
#
# Library for RTS2 JSON calls.
# (C) 2012 Petr Kubanek, Institute of Physics
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

from __future__ import absolute_import

import base64
import json as sysjson
import httplib
import urllib
import re
import string
import os
import socket
import threading

DEVICE_TYPE_SERVERD   = 1
DEVICE_TYPE_MOUNT     = 2
DEVICE_TYPE_CCD       = 3
DEVICE_TYPE_DOME      = 4
DEVICE_TYPE_WEATHER   = 5
DEVICE_TYPE_ROTATOR   = 6
DEVICE_TYPE_PHOT      = 7
DEVICE_TYPE_PLAN      = 8
DEVICE_TYPE_GRB       = 9
DEVICE_TYPE_FOCUS     = 10
DEVICE_TYPE_MIRROR    = 11
DEVICE_TYPE_CUPOLA    = 12
DEVICE_TYPE_FW        = 13
DEVICE_TYPE_AUGERSH   = 14
DEVICE_TYPE_SENSOR    = 15

DEVICE_TYPE_EXECUTOR  = 20
DEVICE_TYPE_IMGPROC   = 21
DEVICE_TYPE_SELECTOR  = 22
DEVICE_TYPE_XMLRPC    = 23
DEVICE_TYPE_INDI      = 24
DEVICE_TYPE_LOGD      = 25
DEVICE_TYPE_SCRIPTOR  = 26

class ChunkResponse(httplib.HTTPResponse):

	read_by_chunks = False

	def __init__(self, sock, debuglevel=0, strict=0, method=None, buffering=False):
		# Python 2.6 does not have buffering
		try:
			httplib.HTTPResponse.__init__(self, sock, debuglevel=debuglevel, strict=strict, method=method, buffering=buffering)
		except TypeError,te:
			httplib.HTTPResponse.__init__(self, sock, debuglevel=debuglevel, strict=strict, method=method)

	def _read_chunked(self,amt):
		if not(self.read_by_chunks):
			return httplib.HTTPResponse._read_chunked(self,amt)
						
		assert self.chunked != httplib._UNKNOWN
		line = self.fp.readline(httplib._MAXLINE + 1)
		if len(line) > httplib._MAXLINE:
			raise httplib.LineTooLong("chunk size")
		i = line.find(';')
		if i >= 0:
			line = line[:i]
		try:
			chunk_left = int(line, 16)
		except ValueError:
			self.close()
			raise httplib.IncompleteRead('')
		if chunk_left == 0:
			return ''
		ret = self._safe_read(chunk_left)
		self._safe_read(2)
		return ret

class Rts2JSON:
	def __init__(self,url='http://localhost:8889',username=None,password=None,verbose=False,http_proxy=None):
		use_proxy = False
		prefix = ''
		if os.environ.has_key('http_proxy') or http_proxy:
			if not(re.match('^http',url)):
				prefix = 'http://' + url
			if http_proxy:
				prefix = url
			url = os.environ['http_proxy']
			use_proxy = True

		url = re.sub('^http://','',url)
		slash = url.find('/')
		if slash >= 0:
			if not(use_proxy):
				prefix = url[slash:]
			self.host = url[:slash]
		else:
			self.host = url

		a = self.host.split(':')
		self.port = 80
		if len(a) > 2:
			raise Exception('too much : separating port and server')
		elif len(a) == 2:
			self.host = a[0]
			self.port = int(a[1])

		self.headers = {'Authorization':'Basic' + string.strip(base64.encodestring('{0}:{1}'.format(username,password)))}
		self.prefix = prefix
		self.verbose = verbose
		self.hlib_lock = threading.Lock()
		self.hlib = self.newConnection()

	def newConnection(self):
		r = httplib.HTTPConnection(self.host,self.port)
		r.response_class = ChunkResponse
		if self.verbose:
			r.set_debuglevel(5)
		return r

	def getResponse(self,path,args=None,hlib=None):
		url = self.prefix + path
		if args:
			url += '?' + urllib.urlencode(args)
		if self.verbose:
			print 'retrieving {0}'.format(url)
		try:
			th = hlib
			if hlib is None:
				self.hlib_lock.acquire()
				th = self.hlib
			th.request('GET', url, None, self.headers)

			r = None
			try:
				r = th.getresponse()
			except httplib.BadStatusLine,ex:
				# try to reload
				th = self.newConnection()
				if hlib is None:
					self.hlib = th
				th.request('GET', url, None, self.headers)
				r = th.getresponse()
			except httplib.ResponseNotReady,ex:
				# try to reload
				th = self.newConnection()
				if hlib is None:
					self.hlib = th
				th.request('GET', url, None, self.headers)
				r = th.getresponse()

			if not(r.status == httplib.OK):
				jr = sysjson.load(r)
				raise Exception(jr['error'])
			return r
		except Exception,ec:
			if self.verbose:
				import traceback
				traceback.print_exc()
				print 'Cannot parse',url,':',ec
			raise ec
		finally:
			if hlib is None:
				self.hlib_lock.release()
	
	def loadData(self,path,args={},hlib=None):
		return self.getResponse(path,args,hlib).read()

	def loadJson(self,path,args=None):
		d = self.loadData(path,args)
		if self.verbose:
			print d
		return sysjson.loads(d)

	def chunkJson(self,r):
		r.read_by_chunks = True
		d = r.read()
		if self.verbose:
			print d
		r.read_by_chunks = False
		return sysjson.loads(d)

class JSONProxy(Rts2JSON):
	"""Connection with managed cache of variables."""
	def __init__(self,url='http://localhost:8889',username=None,password=None,verbose=False,http_proxy=None):
		Rts2JSON.__init__(self,url,username,password,verbose,http_proxy)
		self.devices = {}
	
	def refresh(self,device=None):
		if device is None:
			self.devices = {}
			for x in self.loadJson('/api/devices'):
				self.devices[x] = self.loadJson('/api/get',{'d':x})

		self.devices[device] = self.loadJson('/api/get',{'d':device})['d']

	def getState(self,device):
		return self.loadJson('/api/get',{'d':device})['state']
	
	def getValue(self,device,value,refresh_not_found=False):
		try:
			return self.devices[device][value]
		except KeyError,ke:
			if refresh_not_found == False:
				raise ke
			self.refresh(device)
			return self.devices[device][value]
	
	def setValue(self,device,name,value,async=None):
		values = {'d':device,'n':name,'v':value,'async':async}
		if async:
			values['async'] = async
		self.loadJson('/api/set',values)

	def setValues(self,values,device=None,async=None):
		if device:
			values = dict(map(lambda x:('{0}.{1}'.format(device,x[0]),x[1]),values.items()))
		if async:
			values['async'] = async
		self.loadJson('/api/mset',values)

	def executeCommand(self,device,command):
		ret = self.loadJson('/api/cmd',{'d':device,'c':command,'e':1})
		self.devices[device] = ret['d']
		return ret['ret']
	
	def getDevicesByType(self,device_type):
		return self.loadJson('/api/devbytype',{'t':device_type})

def getProxy():
	global __jsonProxy
	return __jsonProxy

def createProxy(url,username=None,password=None,verbose=False,http_proxy=None):
	global __jsonProxy
	__jsonProxy = JSONProxy(url=url,username=username,password=password,verbose=verbose,http_proxy=http_proxy)
	return __jsonProxy
