
## Quixil is a compact, efficient, class based programming language. 

<br />

At it's core is a bytecode virtual machine just like JVM written in C. Javascript like syntax with a dash of Python.
Take a look at some of the cool stuff.

<br />

```js
var myPlanet = "Earth";     // var is mutable
print("Hello $(myPlanet)")  // OR print "Hello $(myPlanet)";

const solarSystem = ["Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune"];

print("Earth"*3)            // EarthEarthEarth
// Ranges
print(1..6)                 // [1,2,3,4,5,6]

solarSystem.each(p -> {
    if (p.equals(myPlanet)) print(p);
})

func stars() {
    var _2ndNearest = "Proxima Centauri";

    return func() {
        var nearest = "Sun";
        print("After $(nearest) we have $(_2ndNearest)");
    }
}

var whoIsNearest = stars();
whoIsNearest(); // prints After Sun we have Proxima Centauri

```
