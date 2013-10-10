#!/usr/bin/awk -f

BEGIN{}

/)))/ { th[t++] = $2; la[l++] = $3; cr[c++] = $4 }

END{
    print "#run\t\tThroughput\tCommit rate\tLatency"

    for (rows = 0; rows < t; rows += 1) {
	print rows "\t\t" th[rows] "\t\t" la[rows] "\t\t" cr[rows];
	sth += th[rows];
	sla += la[rows];
	scr += cr[rows];
    }
    print "-- Average:";
    print "\t\t" sth/t "\t\t" sla/t "\t\t" scr/t;
}
