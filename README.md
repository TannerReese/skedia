# skedia
Graph functions from the terminal with Intersection calculation.

### Examples
Here is a screenshot of `skedia` graphing an elliptic curve and a vertical sine wave.

![Skedia Demo](https://user-images.githubusercontent.com/14843932/107131989-96d82f00-6898-11eb-8bfe-d8876dd8ddc6.png)

The colored `O` represents the intersection between the curves whose exact coordinates are displayed at the bottom of the screen.

### Help
For more information about usage, use

    $ skedia --help

The can be in two modes; Graphing or Gallery Mode.
When in graphing mode, the cursor movements and key events affect the main graphing window.
When in gallery mode, the cursor movements and key events affect the gallery (list of equations on the left side).

Graphing Controls:
* Arrow Keys / hjkl : Move viewing window around
* Shift Arrows / HJKL : Resize horizontally and vertically
* '=' : Zoom In
* '-' : Zoom Out
* '0' : Return to default Zoom level
* 'n' or 'N' : Find Intersections between Curves
* 'c' or 'C' : Clear All Intersections
* ',' or '<' : Move to prior Intersection
* '.' or '>' : Move to next Intersection
* Control-A (^A) : Switch to Gallery Mode and Create new Equation
* 'g' or 'G' : Switch to Gallery Mode
* Control-C (^C) or Control-Z (^Z) or 'q' or 'Q' : Exit

Gallery Controls:
* All Printable Keys : Type character in Textbox
* Left & Right Arrows : Move within Textbox
* Up & Down Arrows : Move between Textboxes and to Color Picker
* Backspace : Remove character before Cursor
* Home : Go to beginning of Textbox
* End : Go to end of Textbox
* Control-A (^A) : Create new Textbox for Equation at bottom of Gallery
* Control-D (^D) : 	Delete currently selected Textbox and Equation
* Escape : Switch to Graph Mode
* Control-C (^C) or Control-Z (^Z) : Exit

### Design
The textboxs in the skedia gallery can contain curves (to be graphed), definitions of variables, and definitions of functions.
All the standard binary operations of addition `+`, substraction `-`, multiplication `*`, division `/`, and exponentiation `^` are supported.
Also, many standard functions are included such as sine `sin`, cosine `cos`, greatest integer `floor`, least integer `ceil`, hyperbolic sine `sinh`, exponential `exp`, natural logarithm `ln`.

Each curve is of the form `<expression> = <expression>` where an expression may contain references to variables, functions, and graph parameters (e.g. `x` and `y`).
Every curve is treated as an implicit curve. Skedia searches each cell in the grid to find those that contains solutions to the equation.
An example of a curve would be,

    x^2 + y^3 = r * w * f(x)

Here `r` is the radius of the point from the origin, `w` is some variable, and `f` is a function.

Variable definitions are of the form `<variable-name> := <expression>` where the variable name is not defined elsewhere or one of the graph parameters.
These can be used to store constant values or a value which depends on the graph parameters.
An example of a variable definition would be,

    w := x + y - 3

Function definitions are of the form `<function-name>(<arg1-name>, <arg2-name>, ...) := <expression>` where every instance of any argument name in the expression is treated as a reference to that argument.
These can be used to store a function that one wishes to use repeatedly in other expressions.
Note that recursive definitions are not supported.
An example of a function definition would be,

    f(x, t) := sin(k * x + w * t + p)

### Build
To make the `skedia` binary, call

    $ make skedia

**Note:** GCC is the default C compiler used by the makefile

