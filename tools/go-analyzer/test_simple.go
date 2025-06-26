package main

import (
    "fmt"
    "log"
)

func testBranches(x int) {
    if x > 0 {
        log.Println("positive")
    } else {
        fmt.Println("negative or zero")
    }
    
    switch x {
    case 1:
        log.Println("one")
    case 2:
        fmt.Println("two")
    default:
        log.Println("other")
    }
}

func main() {
    testBranches(1)
}
