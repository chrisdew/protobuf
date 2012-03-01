Overview:
---------
This is a fork of https://github.com/chrisdew/protobuf

It was forked to add support for nodejs 0.6.x using code from https://github.com/pzgz/protobuf-for-node
and to provide better support for future nodejs versions and npm.

Prerequisites:
--------------
 - [NodeJS v0.6.X](http://nodejs.org/)
 - [npm](http://npmjs.org/)


To install on Ubuntu:
---------------------

    wget http://protobuf.googlecode.com/files/protobuf-2.4.1.tar.gz
    tar -xzvf protobuf-2.4.1.tar.gz
    cd protobuf-2.4.1/
    ./configure && make && sudo make install
    cd ..
    npm install protobuf

Test your install:
------------------
You can test your installation by opening the nodejs console (by typing `node`) and issue the following

    require('protobuf');
    .exit
    
You should see the output `{ Schema: [Function: Schema] }`

Issues:
-------
Please report any issues or bugs at https://github.com/englercj/protobuf/issues

