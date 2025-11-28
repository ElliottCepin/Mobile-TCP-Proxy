Author: Elliott Cepin | Date: 2025-11-26

# Introduction`
There is a lot about this program that I don't understand, but understanding everything from the outset is not important. If I take this one feature at a time, it should be achieveable without too much trouble.
- telnet connects to cproxy
- telnet sends raw TCP to cproxy
- cproxy sends data to sproxy
- sproxy sends data to telnet daemon
- sproxy sends data to cproxy
- cproxy sends data to telnet
- use polling to support multiple connections

# Telnet connexts to cproxy
Before I can start writing code, I'll need to answer a few questions. The obvious, but imprecise, question is "how?" More narrowly, I need to know how telnet is supposed to send messages to cproxy. My best guess is that it sends them the same way that cproxy will send messages to sproxy. cproxy is running on a port with and has an ip; telnet can send messages to a specified port at a spicific ip address. This is a testable hypothesis, so I will go test it.
After retrieving some code from an old project, I've updated the code of cproxy.c to recieve connections. It successfully recieves connections from telnet in the form `telnet ip port`. This survived testing.

# Telnet sends raw TCP to cproxy
This is only different from the previous section, because the data needs to be "protocolless" as seen by cproxy. For now, we'll just have cproxy print out the amount of bytes it recieved from telnet. In the future, it will just be packaged up and sent across to sproxy.
With a few adjustments, I was able to get cproxy to output the cumulative bytes recieved.

# cproxy sends data to sproxy
This is going to be comparatively difficult, because I haven't had to do any real networking yet. For now, I will use server.c's code for sproxy, and I will pull sections from client.c to send data over. Before beginning, I need to know what arguements cproxy takes.
If it matters, it will be in the spec, so I will check there first.
According to the spec, cproxy has a port that it listens on, as well as the ip & port of sproxy (in that order).
This is the same for sproxy (but the ip and port are for the telnet daemon).


 
