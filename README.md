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
...
Red20: ^ Stmt do { Stmt Stmt } while Expr 
Shift: ^ Stmt do { Stmt Stmt } while Expr ; 
Red15: ^ Stmt Dowh 
Red10: ^ Stmt Ctrl 
Red04: ^ Stmt Stmt 
Shift: ^ Stmt Stmt $ 
Red01: Unit 
ACCEPT Unit 

*** Running ***
1
2
Buzz 3
4
Fizz 5
Buzz 6
7
8
Buzz 9
Fizz 10
11
Buzz 12
13
14
FizzBuzz 15