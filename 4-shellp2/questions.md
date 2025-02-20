1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**:  Using fork makes a child process which gives the parent process (the shell), the ability
to keep running. Calling execvp directly would replace the process of the shell, and commands
would not be able to be inputted

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**:  if the fork syscall fails, it returns -1. My implementation checks for this error,
printing perror, and returns an error code. THe shell then is able to continue running

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**:  the command looks through the PATH variable for directories by iterating through
them until a file is mathces to the command

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**:  calling wait allows for the parent process to but put on hold until the child process
finished. The parents process would continue without it finishing, without handling the childs exit status

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**:  It procies the exit status after wait(). Determines whether a command succeeds or fails. It is essential
for error handling

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**:  When a quote is reached, my implementation collected all characters within the quote
up until it finds another quote, preserving the spaces if any. It is necessary because commands
can involve arguments that have spaces built in them, which would not be processed correctly if it
didn't handle arguments in quotes

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**:  Rather than making a list of commands, i used a command buffer. The biggest challenge was refactoring from collapsing unnecassary
whitespaces to only doing so when it wasn't within quotes.

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**:  Signals act as an alert to the system, where the system determines how to react to them,
which could possibly be changed through implementations. Compared to IPCs, signals are used more
ubruptly. 

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**:  
        >**SIGKILL**: terminates a process immedietly and cannot be blocked. This can be used to stop a process that refuses to stop.
        >**SIGKTERML**: requests to terminate a process, which can be ignored. 
        >**SIGINT**: Uses cntrl C to interrupt a process and can also be chosen to ignore.



- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**:  SIGSTOP basically pauses, or puts a process on hold. It cannot be ignored to ensure
that the process can be paused while it runs
