#!/usr/bin/python

from rts2saf.log import Logger
from rts2saf.rts2exec import Rts2Exec
from rts2.json import JSONProxy

class Args(object):
    def __init__(self):
        pass

args=Args()
args.toPath='/var/log'
args.logfile= 'rts2-debug'
args.level='DEBUG'

debug=False
logger= Logger(debug=debug, args=args ).logger 


proxy=JSONProxy()
ex= Rts2Exec(debug=debug, proxy=proxy, logger=logger)
ex.reeanableEXEC()
logger.info('rts2saf_reenable_exec.py:  reenabled EXEC')
