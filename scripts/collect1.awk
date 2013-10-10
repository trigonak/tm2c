#!/usr/bin/awk -f

BEGIN{}

/Throughput/ { th[t++] = $2; la[l++] = $3; cr[c++] = $4 }

END{
    for (nue = 2; nue < 33; nue *= 2)
	cores[nu++] = nue;
    # cores[nu] = 48;
    cores[nu] = 36;
    
    print "#nue\tThroughput\tCommit rate\tLatency"

    for (rows = 0; rows < 6; rows += 1) {
	printf "%-8d%-12'd%-10.3f%-10.3f\n", 
	    cores[co++], 
	    th[tt++], 
	    la[ll++], 
	    cr[cc++];
    }
}
