This is a fork of http://code.google.com/p/protobuf-for-node/

My intention is just to package it up nicely for NPM and to fix some bugs.

I also intend to make it work with the NodeJS 0.6.x series.



Prerequisites:
--------------

NodeJS v0.4.X
npm


To install on Ubuntu:
---------------------

1. wget http://protobuf.googlecode.com/files/protobuf-2.4.1.tar.gz
2. tar -xzvf protobuf-2.4.1.tar.gz
3. cd protobuf-2.4.1/
4. ./configure && make && sudo make install
5. cd
6. npm install protobuf
7. echo "/home/chris/node_modules/protobuf/build/default/" | sudo tee /etc/ld.so.conf.d/protobuf.conf
   (replace /home/chris/node_modules with wherever you installed the module)
8. sudo ldconfig
9. run node, try "require('protobuf');" - you should see: { Schema: [Function: Schema] }


As seen from the instructions above, this is my first attempt at packaging a slightly complex C++ module for NPM.

If you can help me simplify these instructions, please submit a patch.


Good luck,

Chris.

