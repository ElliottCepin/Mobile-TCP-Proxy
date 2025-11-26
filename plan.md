Author: Elliott Cepin | Date: 2025-11-26

# Introduction
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
