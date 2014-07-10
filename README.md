sabotv2
=======

Same functionality as the first, but this will take more advantage of Qt's features for platform independence.

The main design flaw of the previous version was to try and build a C backend while implementing Qt's C++ GUI on top. 
The backend requires threading and sockets functionality, which the C standard library does not support. This limits 
the old version to POSIX compliant platforms (for pthreads and POSIX sockets). With this drawback, it would be stupid 
to ignore Qt's rich library that supports these features due to my language preference. The new version will make 
use of Qt's sockets and threads libraries which should allow this to be supported on all Qt compliant systems.
