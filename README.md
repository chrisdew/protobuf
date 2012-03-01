This is a fork of https://github.com/chrisdew/protobuf

It was forked to add support for nodejs 0.6.x using code from https://github.com/pzgz/protobuf-for-node

Prerequisites:
--------------

NodeJS v0.6.X
npm


To install on Ubuntu:
---------------------

    wget http://protobuf.googlecode.com/files/protobuf-2.4.1.tar.gz
    tar -xzvf protobuf-2.4.1.tar.gz
    cd protobuf-2.4.1/
    ./configure && make && sudo make install
    cd ..
    npm install protobuf
    node
    require('protobuf');

The final line should output `{ Schema: [Function: Schema] }`.

Please report any issues or bugs at https://github.com/englercj/protobuf/issues

