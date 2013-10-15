var assert = require('assert'),
    puts = require('util').puts,
    read = require('fs').readFileSync,
    Schema = require('../').Schema;

/* hack to make the tests pass with node v0.3.0's new Buffer model */
/* copied from http://github.com/bnoordhuis/node-iconv/blob/master/test.js */
assert.bufferEqual = function(a, b, c) {
  assert.equal(a.length, b.length, c);
  for (var i = 0; i < a.length; i++) {
    assert.equal(a[i], b[i], c);
  }
	assert.equal(
		a.inspect().replace(/^<SlowBuffer/, '<Buffer'),
		b.inspect().replace(/^<SlowBuffer/, '<Buffer'),
                c);
};

var MAX_SAFE_INTEGER = Math.pow(2, 53) - 1;
var MIN_SAFE_INTEGER = -MAX_SAFE_INTEGER;

var T = new Schema(read('test/unittest.desc'))['protobuf_unittest.TestAllTypes'];
assert.ok(T, 'type in schema');
var golden = read('test/golden_message');
var message = T.parse(golden);
assert.ok(message, 'parses message');  // currently rather crashes
// console.log(message)
assert.strictEqual(message.optionalInt64, 102);
assert.strictEqual(message.optionalUint64, 104);
assert.bufferEqual(T.serialize(message), golden, 'roundtrip');

message.ignored = 42;
assert.bufferEqual(T.serialize(message), golden, 'ignored field');

assert.throws(function() {
  T.parse(new Buffer('invalid'));
}, Error, 'Should not parse');

var safeIntegers = [
  MIN_SAFE_INTEGER, MIN_SAFE_INTEGER + 1, MIN_SAFE_INTEGER + 2,
  -100, -1, 0, 1, 100, 500, 1000, 10000, 1000000,
  MAX_SAFE_INTEGER - 2, MAX_SAFE_INTEGER - 1, MAX_SAFE_INTEGER
];
var unsafeIntegers = [
  MIN_SAFE_INTEGER - 1, MIN_SAFE_INTEGER - 2,
  MAX_SAFE_INTEGER + 1, MAX_SAFE_INTEGER + 2,
  '9007199254740992123'
];

safeIntegers.forEach(function (v) {
  assert.strictEqual(T.parse(
    T.serialize({
      optionalInt64: v
    })
  ).optionalInt64, v);

  if (v < 0) {
    return;
  }

  assert.strictEqual(T.parse(
    T.serialize({
      optionalUint64: v
    })
  ).optionalUint64, v);
});

unsafeIntegers.forEach(function (v) {
  assert.strictEqual(T.parse(
    T.serialize({
      optionalInt64: v
    })
  ).optionalInt64, String(v));

  if (v < 0 && typeof v !== 'string') {
    return;
  }

  assert.strictEqual(T.parse(
    T.serialize({
      optionalUint64: v
    })
  ).optionalUint64, String(v));
});

assert.strictEqual(T.parse(
  T.serialize({
    optionalInt32: '3'
  })
).optionalInt32, 3, 'Number conversion');

assert.strictEqual(T.parse(
  T.serialize({
    optionalInt32: ''
  })
).optionalInt32, 0, 'Number conversion');

assert.strictEqual(T.parse(
  T.serialize({
    optionalInt32: 'foo'
  })
).optionalInt32, 0, 'Number conversion');

assert.strictEqual(T.parse(
  T.serialize({
    optionalInt32: {}
  })
).optionalInt32, 0, 'Number conversion');

assert.strictEqual(T.parse(
  T.serialize({
    optionalInt32: null
  })
).optionalInt32, undefined, 'null');

assert.throws(function() {
  T.serialize({
    optionalNestedEnum: 'foo'
  });
}, Error, 'Unknown enum');

assert.throws(function() {
  T.serialize({
    optionalNestedMessage: 3
  });
}, Error, 'Not an object');

assert.throws(function() {
  T.serialize({
    repeatedNestedMessage: ''
  });
}, Error, 'Not an array');

assert.bufferEqual(T.parse(
  T.serialize({
   optionalBytes: new Buffer('foo')
  })
).optionalBytes, new Buffer('foo'));

assert.bufferEqual(T.parse(
  T.serialize({
   optionalBytes: 'foo'
  })
).optionalBytes, new Buffer('foo'));

assert.bufferEqual(T.parse(
  T.serialize({
   optionalBytes: '\u20ac'
  })
).optionalBytes, new Buffer('\u00e2\u0082\u00ac', 'binary'));

assert.bufferEqual(T.parse(
  T.serialize({
   optionalBytes: '\u0000'
  })
).optionalBytes, new Buffer('\u0000', 'binary'));

assert.equal(T.parse(
  T.serialize({
   optionalString: new Buffer('f\u0000o')
  })
).optionalString, 'f\u0000o');

puts('Success');
