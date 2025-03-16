1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

the client detects the end of message marker while reading through data in a loop, which signifies that the server finished sending output. Some ways to handle partial reads and snure complete messages is to use a flength prefix so that the server knows exactly how large the message is. Similarly, a fixed delimiter would help signify exactly when a message ends.

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

The protocol would hae to include a way to append specific boundaries like a delimiter or prefix. Without these,a receiver could mishandle messages through unexpected splitting or combining

3. Describe the general differences between stateful and stateless protocols.

sstateful protocols keeps track of information between requests in a session so requests can depend on each other while stateless proccesses each requests without keeping track of interactions so that a new interaction willnever rely on and old one. 

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

We would use it because it supports high-speed interactions allowing for low overhead and latency. 

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

The OS provides the ability to uses socks where there is compatability to do things like send, receive and connect in order to support communication.
