if (typeof Array.prototype.forEach != 'function') {
    Array.prototype.forEach = function(callback){
      for (var i = 0; i < this.length; i++){
        callback.apply(this, [this[i], i, this]);
      }
    };
}

if(typeof String.prototype.trim !== 'function') {
  String.prototype.trim = function() {
    return this.replace(/^\s+|\s+$/g, ''); 
  }
}

[].filter || (Array.prototype.filter = // Use the native array filter method, if available.
  function(a, //a function to test each value of the array against. Truthy values will be put into the new array and falsy values will be excluded from the new array
    b, // placeholder
    c, // placeholder 
    d, // placeholder
    e // placeholder
  ) {
      c = this; // cache the array
      d = []; // array to hold the new values which match the expression
      for (e in c) // for each value in the array, 
        ~~e + '' == e && e >= 0 && // coerce the array position and if valid,
        a.call(b, c[e], +e, c) && // pass the current value into the expression and if truthy,
        d.push(c[e]); // add it to the new array
      
      return d // give back the new array
  })

Object.keys = Object.keys || 
    function ( 
        o, // object
        k, // key
        r  // result array
    ){
        // initialize object and result
        r=[];
        // iterate over object keys
        for (k in o) 
            // fill result array with non-prototypical keys
            r.hasOwnProperty.call(o, k) && r.push(k);
        // return result
        return r
    }