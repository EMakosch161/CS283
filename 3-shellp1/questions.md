1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**:  For this application, fgets() is ideal because it helps prevent buffer overflows by reading input by line. It also limits the number of character read at once

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**:  Using malloc makes it easier to free memory when needed, and also creates the ability for dynamic memery allocation. 


3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**:  Trimming the leading and trailing spaces allow for easiest command parsing, and prevents misinterpretation of commands that whitespaces may cause and lead to matching command name errors.

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:  Redirection is the process of changing the destination of STDIN, STDOUT, or STDerror to another source. 3 examples would be reading inputs from a file, directing outputs into a file, or sending STDERR into a file of error messages. The difficult task if dealing with possible errors that may be experienced with using rediration such as an unreachable file.

- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**:  Redirection is the process of sending or reading STDIN/ STDOUT/ STDerror from or to another file. A pipe runs down a chain of commands so that the output of the prior command is what the current one uses as input. 

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**:  When working with both STDERR and STDOUT, STDOUT typically refers to a normal output while an error refers to an error message follow an unsuccessful action. Keeping these two distinct allows users to redirect the two into different sources. This makes logging errors and real output simpler

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**:  Both actions should display distinctly from one another to make it easier on the user. This would be done by giving the user the ability to reassign where the STDOUT and STDERR go, giving them the ability to merge. This can be done by creating a command that send both into one file.
