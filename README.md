This is a fork of https://github.com/chrisdew/protobuf

It was forked to add support for nodejs 0.6.x using code from https://github.com/pzgz/protobuf-for-node

Prerequisites:
--------------

NodeJS v0.6.X
npm


To install on Ubuntu:
---------------------

1. wget http://protobuf.googlecode.com/files/protobuf-2.4.1.tar.gz
2. tar -xzvf protobuf-2.4.1.tar.gz
3. cd protobuf-2.4.1/
4. ./configure && make && sudo make install
5. cd
6. npm install protobuf
7. run node, try "require('protobuf');" - you should see: { Schema: [Function: Schema] }

Please report any issues or bugs at https://github.com/englercj/protobuf/issues

