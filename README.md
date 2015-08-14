```
    $ ./interp tests/fizzbuzz.txt 
    *** Lexing ***
    number = 1;

    do {
        if (number % 3 == 0 && number % 5 == 0) {
            print "FizzBuzz " number;
        } elif (number % 5 == 0) {
            print "Fizz " number;
        } elif (number % 3 == 0) {
            print "Buzz " number;
        } else {
            print number;
        }
        
        number = number + 1;
    } while (number < 100);

    *** Parsing ***
    Shift: ^ 
    Shift: ^ number 
    Shift: ^ number = 
    Shift: ^ number = 1 
    Red18: ^ number = Atom 
    Red19: ^ number = Expr 
    Shift: ^ number = Expr ; 
    Red05: ^ Assn 
    Red02: ^ Stmt 
    Shift: ^ Stmt do 
    Shift: ^ Stmt do { 
    Shift: ^ Stmt do { if 
    Shift: ^ Stmt do { if ( 
    Shift: ^ Stmt do { if ( number 
    Red17: ^ Stmt do { if ( Atom 
    Red19: ^ Stmt do { if ( Expr 
    Shift: ^ Stmt do { if ( Expr % 
    Shift: ^ Stmt do { if ( Expr % 3