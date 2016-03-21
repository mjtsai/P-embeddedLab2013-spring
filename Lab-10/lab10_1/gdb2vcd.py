import re

def main():
	qTaskName=[];
	qTickCount=[];

	fptr = open("gdb.log", "r");
	for line in fptr:
		line = line.rstrip();
#		print "["+line+"]";
		found = re.search('pxCurrentTCB->pcTaskName = \"(([^\"](?!000))*)', line);
		if(found):
			taskName = found.group(1);
			taskName = taskName.replace(' ', '_');			
			print "pcTaskName = "+taskName;
			qTaskName.append(taskName);
		found = re.search('xTickCount = (.*)', line);
		if(found):
#			print "xTickCount = "+found.group(1);
			qTickCount.append(found.group(1));
	fptr.close();
	#
	print qTaskName;
	print qTickCount;

	fptr = open("sched.vcd", "w");
	fptr.write("$version\n");
	fptr.write("$end\n");
	fptr.write("$timescale 1ms\n");
	fptr.write("$end\n");
	# declare vars
	fptr.write("$var wire 1 Tick Tick $end\n");
	qTaskNameNoDup = list(set(qTaskName));
	for i in range(len(qTaskNameNoDup)) :
		fptr.write("$var wire 1 %s %s $end\n" % (qTaskNameNoDup[i], qTaskNameNoDup[i]) );
	
	fptr.write("$dumpvars\n");
	# initialize value
	fptr.write("#0\n");
	fptr.write("b0 %s\n" % "Tick" );	
	for i in range(len(qTaskNameNoDup)) :
		fptr.write("b0 %s\n" % qTaskNameNoDup[i] );

	# print collection
	# collect vars with same time
	curTick = qTickCount[0];
	curTaskCol = [];
	for i in range(len(qTickCount)) :
		if( qTickCount[i] == curTick ):
			curTaskCol.append( qTaskName[i] );
		else:
			fptr.write("#%d\n" % (int(curTick)*10) );
			fptr.write("b1 %s\n" % "Tick" );
			for j in range(len(curTaskCol)):
				fptr.write("b1 %s\n" % curTaskCol[j] );
			fptr.write("#%d\n" % (int(curTick)*10+1) );
			fptr.write("b0 %s\n" % "Tick" );
			for j in range(len(qTaskNameNoDup)) :
				fptr.write("b0 %s\n" % qTaskNameNoDup[j] );

			curTaskCol = [ qTaskName[i] ];
			curTick = qTickCount[i];
			
	fptr.write("$end\n");
	fptr.close();

if __name__ == "__main__":
    main()
