//@ skip if $memoryLimited

function foo(a) {
  arguments;
  a.length;
}

let x = new Uint8Array();
for (let i = 0; i < testLoopCount; ++i)
  foo(x);

try {
    foo(new Uint8Array(2**31));
} catch { }
