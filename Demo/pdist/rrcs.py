#! /usr/local/bin/python

import sys
import os
import getopt
import string
import md5
import tempfile

def main():
	sys.stdout = sys.stderr
	try:
		opts, rest = getopt.getopt(sys.argv[1:], 'h:p:qv')
		if not rest:
			raise getopt.error, "missing command"
		cmd, rest = rest[0], rest[1:]
		if not commands.has_key(cmd):
			raise getopt.error, "unknown command"
		coptset, func = commands[cmd]
		copts, files = getopt.getopt(rest, coptset)
	except getopt.error, msg:
		print msg
		print "usage: rrcs [options] command [options] [file] ..."
		print "where command can be:"
		print "      ci|put      # checkin the given files"
		print "      co|get      # checkout"
		print "      info        # print header info"
		print "      head        # print revision of head branch"
		print "      list        # list filename if valid"
		print "      log         # print full log"
		print "      diff        # diff rcs file and work file"
		print "if no files are given, all remote rcs files are assumed"
		sys.exit(2)
	x = openclient(opts)
	if not files:
		files = x.listfiles()
	for fn in files:
		try:
			func(x, copts, fn)
		except (IOError, os.error), msg:
			print "%s: %s" % (fn, msg)

def openclient(opts):
	import client
	import RCSProxy
	host = 'spam'
	port = 4127
	verbose = client.VERBOSE
	for o, a in opts:
		if o == '-h':
			host = a
			if ':' in host:
				i = string.find(host, ':')
				host, p = host[:i], host[i+1:]
				if p:
					port = string.atoi(p)
		if o == '-p':
			port = string.atoi(a)
		if o == '-v':
			verbose = verbose + 1
		if o == '-q':
			verbose = 0
	address = (host, port)
	x = RCSProxy.RCSProxyClient(address, verbose)
	return x

def checkin(x, copts, fn):
	f = open(fn)
	data = f.read()
	f.close()
	new = not x.isfile(fn)
	if not new and same(x, copts, fn, data):
		print "%s: unchanged since last checkin" % fn
		return
	message = asklogmessage(new)
	messages = x.put(fn, data, message)
	if messages:
		print messages

def checkout(x, copts, fn):
	data = x.get(fn)
	f = open(fn, 'w')
	f.write(data)
	f.close()

def info(x, copts, fn):
	dict = x.info(fn)
	keys = dict.keys()
	keys.sort()
	for key in keys:
		print key + ':', dict[key]
	print '='*70

def head(x, copts, fn):
	head = x.head(fn)
	print fn, head

def list(x, copts, fn):
	if x.isfile(fn):
		print fn

def log(x, copts, fn):
	flags = ''
	for o, a in copts:
		flags = flags + ' ' + o + a
	flags = flags[1:]
	messages = x.log(fn, flags)
	print messages

def diff(x, copts, fn):
	if same(x, copts, fn):
		return
	flags = ''
	for o, a in copts:
		flags = flags + ' ' + o + a
	flags = flags[1:]
	data = x.get(fn)
	tfn = tempfile.mktemp()
	try:
		tf = open(tfn, 'w')
		tf.write(data)
		tf.close()
		print 'diff %s -r%s %s' % (flags, x.head(fn), fn)
		sts = os.system('diff %s %s %s' % (flags, tfn, fn))
		if sts:
			print '='*70
	finally:
		remove(tfn)

def same(x, copts, fn, data = None):
	if data is None:
		f = open(fn)
		data = f.read()
		f.close()
	lsum = md5.new(data).digest()
	rsum = x.sum(fn)
	return lsum == rsum

def asklogmessage(new):
	if new:
		print "enter description,",
	else:
		print "enter log message,",
	print "terminate with single '.' or end of file:"
	if new:
		print "NOTE: This is NOT the log message!"
	message = ""
	while 1:
		sys.stderr.write(">> ")
		sys.stderr.flush()
		line = sys.stdin.readline()
		if not line or line == '.\n': break
		message = message + line
	return message

def remove(fn):
	try:
		os.unlink(fn)
	except os.error:
		pass

commands = {
	'ci': ('', checkin),
	'put': ('', checkin),
	'co': ('', checkout),
	'get': ('', checkout),
	'info': ('', info),
	'head': ('', head),
	'list': ('', list),
	'log': ('bhLRtd:l:r:s:w:V:', log),
	'diff': ('c', diff),
	}

main()
