1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

_answer here_
the shell works by saving the ID of each child process and uses waitpid() to wait for all of them to finish. Without this, child processes may still run or have an exit status waiting to process. Without, it could crash due to leaks or faulty output

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?

_answer here_
using dup2 keeps the original file descriptors open. Closing them is necassary for the system to understand that its done with that pipe end. Not closing them could possibly result in an error occuring that theres not EOF seen

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?

_answer here_
cd is an internal built in command because it needs to effect the shell itself by changing the cwd. Externally running cd would create an entirely new process and a cd command would only change it within that process rather than the shell itself

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

_answer here_
A dynamically allocated array would be swapped for the fixed array and using mallocs. This would be beneficial for running any number of piped commands but the fault is harder memory management
