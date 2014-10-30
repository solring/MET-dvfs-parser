from sys import argv, maxint
from os import path, listdir

class Record:
	def __init__(self, time, level1, level2, load):
		self.time = time
		self.l1 = level1
		self.l2 = level2
		self.load = load

class Load:
	def __init__(self, time, util, level):
		self.time = time
		self.level = level
		self.util = util
		
class File:
	def __init__(self, records, accu, count):
		self.records = records
		self.loads = []
		self.count = count
		self.accu = accu

sample_rate = 1000
		
def parseDVFSSwitch(fd):

	res = []
	start = last = 0
	count = 0
	swts_accu = {}

	for line in fd.readlines():
	
		# input pre-process 
		tokens = line.strip().split(',')
		if len(tokens) < 2: continue		
		try:
			time = float(tokens[0])
			freq = float(tokens[1])
		except:
			print tokens
			continue

		if last == 0: # the begin of trace
			start = time
			last = freq
			res.append( Record(start, -1, freq, 0) )
			continue
			
		if freq != last:
			
			# accumulate the number of switch  
			key = "%f-%f" %(last, freq)
			if key not in swts_accu: swts_accu[key] = 0
			swts_accu[key] += 1
			
			# record switching to which level
			res.append( Record(time, last, freq, 0) )
			
			last = freq
			count += 1
	
	return File(res, swts_accu, count)
	
def parseUtil(file, fdload):
	records = file.records
	maxlen = len(records)
	print maxlen
	
	ptr = 1 # skip the first record which does not indicate a switch of freq.
	timestamp = 0
	util = 0.0
	last_swt_t = 0
	period = 1.0/sample_rate
	swt_t = maxint
	if records and maxlen > 1: 
		swt_t = records[ptr].time
	
	accu = 0.0
	
	for line in fdload.readlines():
		
		# pre-process of input
		tokens = line.strip().split(',')
		if len(tokens) < 2: continue
			
		try: 
			timestamp = float(tokens[0])
			util = float(tokens[1])
		except: 
			print "parseUtil: cannot turn str to float"
			print tokens 
			continue
		
		if last_swt_t == 0: # begin of load record
			last_swt_t = timestamp
			
		
		
		# determine level switch
		if timestamp > swt_t and ptr < maxlen: # switch!
			print "switch!"
			
			# calculate util. at the last level
			diff = swt_t - last_swt_t
			if diff < period: # switch faster than sample period
				records[ptr].load = accu
			else:
				records[ptr].load = (accu / (diff * sample_rate))
				
			accu = util
			ptr += 1
			
			# check if finished
			if ptr < maxlen:
				last_swt_t = swt_t
				swt_t = records[ptr].time

		else:
			accu += util
			
		# append load and DVFS detail
		file.loads.append( Load(timestamp, util, records[ptr-1].l2) ) # we need the last freq before switch
	
	# append the average util after the last switch
	last_level = records[-1].l2
	diff = timestamp - swt_t
	if diff < period:
		util = accu
	else:
		util = accu / (diff * sample_rate)
		
	records.append( Record(-1.0, last_level, -1.0, util) )
			
		
def outputResult(fdout, flist):
	for key, val in flist.items():
	
		# Summary
		fdout.write("%s\n" % key)
		fdout.write("Switch,Count\n")
		for k, v in val.accu.items():
			fdout.write("%s, %d\n" %(k, v))
		
		fdout.write("total,%d\n" % val.count)
		
		fdout.write("\n")
		
		# Detail
		fdout.write("Time,Freq1,Freq2,Load\n")
		for r in val.records:
			fdout.write("%f,%f,%f,%f\n" % (r.time, r.l1, r.l2, r.load) )
			
		fdout.write("\n")	
		
		# Detailed load & freq
		fdout.write("Time,Load,Freq Level\n")
		for r in val.loads:
			fdout.write("%f,%f,%f\n" % (r.time, r.util, r.level))
		fdout.write("\n")
	
	
###### start of main ######

if len(argv) < 3:
	print "usage: python parseDFSswt.py [input directory] [sample rate]"
	exit(0)

	
targetdir = argv[1]	
sample_rate = int(argv[2])
if not sample_rate: sample_rate = 50

outfilename = "Summary.csv"
outputfile = path.join(targetdir, outfilename)

filelist = {}
total = 0

with open(outputfile, "w") as fdout:
	
	# for every DVFS files in the directory
	for f in listdir(targetdir):
		if f==outfilename: continue
		if ".csv" not in f or "DVFS" not in f: continue

		print "Parsing %s" % f
		# open DVFS file
		with open(path.join(targetdir, f), "r") as fd:
			
			#filelist[f] = []
			record = []
			
			
			# parse records
			#result, swtaccu, swtcount = parseDVFSSwitch(record)
			filelist[f] = parseDVFSSwitch(fd)
			total += filelist[f].count
		
		
		
		# open load file
		try:
			#loadfilename = "LOAD-" + f.split('-')[1]
			loadfilename = f.replace("DVFS", "LOAD").replace(" (kHz)", "")
		except:
			print "error: DVFS file name format error: "%loadfilename
			continue
		
		with open(path.join(targetdir, loadfilename), "r") as fdload:
			parseUtil(filelist[f], fdload)
	
	#fdout.write("total switches, %d\n" % total)
	outputResult(fdout, filelist)
		
fdout.close()		
