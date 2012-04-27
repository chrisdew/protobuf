This is a fork of http://code.google.com/p/protobuf-for-node/

It now works with the NodeJS 0.6.x series.

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

For Ubuntu, the following step may be unneeded now.

    echo "/home/chris/node_modules/protobuf/build/Release/" | sudo tee /etc/ld.so.conf.d/protobuf.conf
    (replace /home/chris/node_modules with wherever you installed the module)

For Ubuntu, update library paths.

    sudo ldconfig

For OSX, you might need to add the path:

    export DYLD_LIBRARY_PATH=/home/chris/node_modules/protobuf/build/Release:/usr/local/lib:$DYLD_LIBRARY_PATH

And test that it works...  Run node, try 

    require('protobuf');"

you should see: 

    { Schema: [Function: Schema] }


As seen from the instructions above, this is my first attempt at packaging a slightly complex C++ module for NPM.

If you can help me simplify these instructions, please submit a patch.


Good luck,

Chris.

