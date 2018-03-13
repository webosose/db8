Summary
-------
A tool that collect performance metrics of the gtest application
and plot graphs.

How to run
----------
Run './run.sh --help' to list all possible command-line options and their description.
    $ ./run.sh --help
    Usage: ./run.sh [--remote-host=HOST] [--remote-user=USER] [--remote-fs=PATH] [--prefix=PATH] [--start=NUM] [--end=NUM] [--step=NUM] [--make-plots=true|false] [--make-dumps] [--list-tests] [--db-representation=TYPE] TEST_NAME...
    Options:
        --remote-host=HOST        Copy itself to remote host and run. If not specified, runs locally
        --remote-user=USER        Remote user's name
                                  Default: root
        --remote-fs=PATH          Remote location
                                  Default: /home/root
        --prefix=PATH             The prefix to override database directory (useful if you want to run test in tmpfs)
                                  Default: current working directory
        --start=NUM               The amount of test database entries to begin with (adjective to 10)
                                  Default: 10
        --end=NUM                 Maximum amount of database entries (adjective to 10)
                                  Default: 1010
        --step=NUM                With each step, the amount of entries increases by NUM
                                  Default: 1000
        --make-plots=true|false   Make plots from test results
                                  Default: true
        --make-dumps              Make test databases represented as json
        --list-tests              List tests to be executed and exit
        --db-representation=TYPE  Set how the database will be represented. Can be 'folder' or 'dump'
                                  Default: dump
        TEST_NAME..               The test name to run
                                  Default: all 'Sequential' tests enabled in gtest_dbclient_perf
        --help                    Print this help and exit

Examples
--------
Execution with no options will give you the result of testing against
two databases with 10 and 1010 entries. The test database will be stored
in gtest_dbclient_perf working directory.
For example:

    $ ./run.sh
    # dbclient-perf-data-1
    # start=10, end=1010, step=1000
    # nrecords   cpu_ktime   cpu_utime   mem_avg     mem_peak
    10           0.001       0.005       117516      117516
    1010         0.060       0.428       117768      118020
    # dbclient-perf-data-1
    # start=10, end=1010, step=1000
    # nrecords   cpu_ktime   cpu_utime   mem_avg     mem_peak
    10           0.001       0.002       117516      117516
    1010         0.061       0.209       118016      118020
    # dbclient-perf-data-1
    # start=10, end=1010, step=1000
    # nrecords   cpu_ktime   cpu_utime   mem_avg     mem_peak
    10           0.001       0.007       117516      117516
    1010         0.065       0.521       118324      118636
    # dbclient-perf-data-1
    # start=10, end=1010, step=1000
    # nrecords   cpu_ktime   cpu_utime   mem_avg     mem_peak
    10           0.001       0.004       117516      117516
    1010         0.001       0.005       118032      118036

Test database parameters may be defined with --start, --end and --step arguments.
Make sure that --start and --end are adjective to 10.
For example:

    $ ./run.sh --start=1010 --end=50010 --step=10000

will make 6 tests with databases containing 1010, 11010, 21010, 31010, 41010
and 51010 entries.

Test execution time may be decreased if user specifies a tmpfs folder as a prefix.
For example:

    $ mount | grep rw | grep mode=0755
    udev on /dev type devtmpfs (rw,mode=0755)
    tmpfs on /run type tmpfs (rw,noexec,nosuid,size=10%,mode=0755)
    none on /run/user type tmpfs (rw,noexec,nosuid,nodev,size=104857600,mode=0755)
    $ ls /run/user
    1000
    $ ./run.sh --prefix=/run/user/1000

If you want to run gtest_dbclient_perf on remote host, provide its IP address or
domain name as an argument. The program will try to log in to the remote host as root.
For example:

    $ ./run.sh --remote-host=192.168.0.1

