const z = {};
Object.defineProperty(z, 'xxx', { set: isFinite });

function foo() {
  z.xxx = undefined;
}
noInline(foo);

for (let i=0; i<testLoopCount; i++) {
    foo();
}
