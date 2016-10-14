To compile the project, simply run "make" in the root folder. 
There are two test TRS servers in TRS_Languages/, one in English and the other
in German. These are just symbolic links to the executable in the root
directory, so you only need to "make" once.

To exit any of the programs, simply type "exit".

You may also choose to simply send a SIGINT to terminate a TRS instance (the result will
be the same as if you type "exit").

Any remaining functionality will be the same as originally requested.

TRS requires a file called "text_translation.txt" and a file called
"file_translation.txt" in the same directory as the executable in order to
function properly. To start a TRS in a different language, it should be
executed in a different directory.

The files util.c and util.h in each specific executable source code contain
shared functions and are therefore identical.
