# Contributing to ATX
This document goes over the contribution standard for ATX.

## Code style

### Bracket placement
Squiggly brackets are placed after the parenthesis:
```c
if(i == 0) {
  // ...
}
```

### Keyword placement
Instead of following Plan9's `style`, you declare functions like this:
```c
// if parameters are over 80 chars, split new line like so
void foo(int bar, int bar2, int bar3, int faz, int foo,
         int last_argument);

int main(void) {

}
```
