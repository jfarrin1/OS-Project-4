Project 4 - Synchronization
---------------------------

Partners: James Farrington and Jose Badilla

Our program utilizes two files: a driver file called site-tester.cpp and a
curl.h header file.

The site-tester.cpp sets the default values of our parameters: PERIOD_FETCH,
NUM_FETCH, NUM_PARSE, SEARCH_FILE, SITE_FILE. Using a global mutex and
condition variable we ensure that multiple threads take turns sharing the
same resource (namely, the parse and fetch queues, which must only be
accessed by a single thread at a time). These threads search for the occurences
of the words specified in the SEARCH_FILE in the sites specified in the SITE_FILE.
They then take turn writing these occurences to a csv, or comma-separated value
file. The curl.h file was simply used to obtain the HTML response from the URL 
passed to it.

Upon ensuring that the configuration file was provided as the singular
argument, the driver program parses the contents of the file (one parameter per 
line in the format of 'PARAMETER=VALUE') to isolate the values. It then changes 
the parameter values in the program to those specified in the configuration file.
This part of the code also handles erroneous values. We also verify that the file
streams pertaining to the opened SEARCH_FILE and SITE_FILE are error-free
(e.g. we ensure that the file exists).

We have implemented the use of various signals in order to ensure that the
system issues a new set of requests after a particular period (SIGALRM),
that it exits upon receiving an interrupt signal (SIGINT), and that it
exits when its controlling terminal is closed (SIGHUP).

One of the main problems we faced was understanding how to lock and unlock the mutex
without perpetually blocking the other threads' access to the fetch and parse queues.
We fixed this by accessing these queues within while loops that ran until the
interrupt signal was sent.
