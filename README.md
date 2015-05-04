TM2C is the most portable software transactional memory (STM) to date. TM2C is distributed: its design is fully decentralized and builds on top of message passing. Nevertheless, TM2C targets large-scale many-cores, not distributed systems. Although decentralized (and therefore highly scalable), TM2C guarantees the termination of every transaction, unlike other distributed STMs.

* **Website**             : http://lpd.epfl.ch/site/tm2c
* **Author**              : Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
* **Related Publications**: TM2C is now a part of the SSYNC synchronization suite (http://lpd.epfl.ch/site/ssync)
  1. *TM2C: a Software Transactional Memory for Many-Cores*, 
     Vincent Gramoli, Rachid Guerraoui, Vasileios Trigonakis (alphabetical order), 
     EuroSys '12 - Proceedings of the 7th ACM European conference on Computer Systems
  2. *Everything You Always Wanted to Know about Synchronization but Were Afraid to Ask*, 
     Tudor David, Rachid Guerraoui, Vasileios Trigonakis (alphabetical order), 
     SOSP '13 - Proceeding of the 24th ACM Symposium on Operating Systems Principles


Installation:
-------------

Please refer to the INSTALL file.


Details:
--------

TM2C brings the client-server design to STMs. The STM is actually provided as a service (by what we 
call the service processes) to the application processes. Consequently, a transactional request in
TM2C happens over message passing. 

The distributed service (DSL) is able to perform normal contention management upon a conflict, similar
to centralized STMs. However, each DSL process is able to take its decisions locally, without the need
to utilize any global data or to communicate with other DSL processes. This decentralization is
important for the scalability of the system on large scale processors. TM2C comes with three contention
managers:

* *Offset-Greedy*: uses timestamps as criteria. Older transaction have priority over newer. Offset-Greedy is a decentralized implementation of Greedy.
* *Wholly*: uses the number of committed transactions as the criterion. The processes with fewer committed transactions have priority.
* *FairCM*: uses the effective transactional time of each process as the criterion. This corresponds to the time a process has spent on successful transactions. The process with the lower time has priority over the others.


Limitations:
------------

The current implementation of TM2C bundles the DSL service with the application, i.e., they are both
initialized together. In other words, it is currently not possible to initiate the DSL service and 
let an application start later. Ideally, we would like to allow the application contact the DSL
service and then be able to utilize it. 
