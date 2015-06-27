# What’s Memo #
Memo is an open source C++ library that provides data-driven and object-oriented memory management.
The classic scenario of dynamic memory allocation consists of a program requesting randomly dynamic storage to a black-box allocator that implements a set of malloc\realloc\free functions, and that doesn’t know and can’t predict anything about the requests of the program.
Memo adds a layer between the allocator and the program, and allows the program to select the best memory allocation strategy with the best tuning for every part of the program.

You can find an overview of memo at
http://peggysansonetti.it/tech/memo%20documentation/index.html