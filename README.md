This is a fork of http://code.google.com/p/protobuf-for-node/

It now works with the NodeJS 0.8.x series.

To install, just type:

    npm install protobuf

Thanks to work by seishun, it now uses node-gyp and has google's protocol bufferes library integrated, so no separate installtion required.

All the best,

Chris.

P.S. Breaking change in 0.8.6:
uint64 and int64 are now read as Javascript Strings, rather than floating point numbers.  They can still be set from Javascript Numbers (as well as from string).

P.P.S. Here's an example I did for https://github.com/chrisdew/protobuf/issues/29 - most users won't need the complication of `bytes` fields.

buftest.proto

    package com.chrisdew.buftest;

    message BufTest {
      optional float num  = 1;
      optional bytes payload = 2;
    }

buftest.js

    var fs = require('fs');
    var Schema = require('protobuf').Schema;

    // "schema" contains all message types defined in buftest.proto|desc.
    var schema = new Schema(fs.readFileSync('buftest.desc'));

    // The "BufTest" message.
    var BufTest = schema['com.chrisdew.buftest.BufTest'];

    var ob = { num: 42 };
    ob.payload = new Buffer("Hello World");

    var proto = BufTest.serialize(ob);
    console.log('proto.length:', proto.length);

    var outOb = BufTest.parse(proto);
    console.log('unserialised:', JSON.stringify(outOb));

    var payload = new Buffer(outOb.payload);
    console.log(payload);

Makefile: (second line begins with a TAB not spaces)

    all:
        protoc --descriptor_set_out=buftest.desc --include_imports buftest.proto


output:

    $ node buftest.js 
    proto.length: 18
    unserialised: {"num":42,"payload":{"0":72,"1":101,"2":108,"3":108,"4":111,"5":32,"6":87,"7":111,"8":114,"9":108,"10":100,"length":11}}
    payload: <Buffer 48 65 6c 6c 6f 20 57 6f 72 6c 64>









Older instructions for use with the NodeJS 0.6.x series.
========================================================

Prerequisites:
--------------

NodeJS v0.6.X
npm


To install on Ubuntu and OSX:
-------------------------------

The first steps are to build and install Google's protobuf library. Make sure you have the right version by running "protoc --version" after the install.

    wget http://protobuf.googlecode.com/files/protobuf-2.4.1.tar.gz
    tar -xzvf protobuf-2.4.1.tar.gz
    cd protobuf-2.4.1/
    ./configure && make && sudo make install
    cd

This installs the npm package.

    npm install protobuf

For Ubuntu, update library paths.

    sudo ldconfig

For OSX, you might need to add the path:

    export DYLD_LIBRARY_PATH=/home/chris/node_modules/protobuf/build/Release:/usr/local/lib:$DYLD_LIBRARY_PATH

And test that it works...  Run node, try 

    require('protobuf');

you should see: 

    { Schema: [Function: Schema] }


As seen from the instructions above, this is my first attempt at packaging a slightly complex C++ module for NPM.

If you can help me simplify these instructions, please submit a patch.


Good luck,

Chris.

