#!/usr/bin/awk -f

BEGIN{}

/)))/ { th[t++] = $2; la[l++] = $3; cr[c++] = $4 }

END{
    for (rows = 0; rows < t; rows += 1) {
	sth += th[rows];
	sla += la[rows];
	scr += cr[rows];
    }
    print ")))\t\t" sth/t "\t\t" sla/t "\t\t" scr/t "\tThroughput";
}
