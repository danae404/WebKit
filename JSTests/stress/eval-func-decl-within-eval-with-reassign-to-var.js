var assert = function (result, expected, message) {
    if (result !== expected) {
        throw new Error('Error in assert. Expected "' + expected + '" but was "' + result + '":' + message );
    }
};

var assertThrow = function (cb, expected) {
    let error = null;
    try {
        cb();
    } catch(e) {
        error = e;  
    }
    if (error === null) {
        throw new Error('Error is expected. Expected "' + expected + '" but error was not thrown."');
    }
    if (error.toString() !== expected) {
        throw new Error('Error is expected. Expected "' + expected + '" but error was "' + error + '"');
    }
};

function foo() {
    {
        var f = 20;
        eval('var f = 15; eval(" { function f() { }; } ")');
        assert(typeof f, "function");
    }
    assert(typeof f, "function", "#1");
}

for (var i = 0; i < testLoopCount; i++) {
    foo();
    assertThrow(() => f, "ReferenceError: Can't find variable: f");
}

function boo() {
    {
        var l = 20;
        eval('{ let l = 15; eval(" { function l() { }; } ")}');
        assert(l, 20, "#3");
    }
    assert(typeof l, 'number', "#4");
}

for (var i = 0; i < testLoopCount; i++){
    boo();
    assertThrow(() => l, "ReferenceError: Can't find variable: l");
}

function foobar() { 
    eval("if (false) { function _bar() { } }"); 
    assert(_bar, undefined); 
}

for (var i = 0; i < testLoopCount; i++){
    foobar();
    assertThrow(() => _bar, "ReferenceError: Can't find variable: _bar");
}

// Current test does not work because it should raise exception
// that f could not be redeclared
function goo() {
    {   
        var error = false;
        try {
            let f = 20;
            eval('var f = 15; eval(" { function f() { }; } ")');
        } catch (e) {
            error = e instanceof SyntaxError;
        }
        assert(error, true, "syntax error should be raisen");
    }
    assert(typeof f, "undefined", "#6");
}

for (var i = 0; i < testLoopCount; i++) {
    goo();
    assert(typeof f, "undefined", "#7");
}

function hoo() {
    {
        let h = 20;
        try { eval('var h = 15;'); } catch (e) { var evalError = e; }
        assert(evalError.toString(), "SyntaxError: Can't create duplicate variable in eval: 'h'");
        eval('eval("if (false) { function h() {} } ");');
        assert(h, 20);
    }
    assert(typeof h, "undefined");
}

for (var i = 0; i < testLoopCount; i++) {
    hoo();
    assertThrow(() => h, "ReferenceError: Can't find variable: h");
}

function joo() {
    {
        var h = 20;
        eval('var h = 15; eval(" if (false){ function h() { }; } ")');
        assert(typeof h, "number");
    }
    assert(typeof h, "number", "#10");
}

for (var i = 0; i < testLoopCount; i++){
    joo();
    assertThrow(() => h, "ReferenceError: Can't find variable: h");
}

function koo() {
    {
        var k = 20;
        eval('var k = 15; eval(" if (true){ function k() { }; } ")');
        assert(typeof k, "function" );
    }
    assert(typeof k, "function", "#12");
}

for (var i = 0; i < testLoopCount; i++){
    koo();
    assertThrow(() => h, "ReferenceError: Can't find variable: h");
}

function loo() { 
    let error;
    try {
        let h = 20;
        eval("var h; if (false) { function h() { } }");
    } catch (e) {
        error = e;
    }

    assert(`${error}`, "SyntaxError: Can't create duplicate variable in eval: 'h'", "#13");
}

for (var i = 0; i < testLoopCount; i++) {
    loo();
    assertThrow(() => h, "ReferenceError: Can't find variable: h");
}
