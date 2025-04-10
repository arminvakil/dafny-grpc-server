predicate even(x:int) {
  x % 2 == 0
}

lemma foo(x:int)
  requires even(x)
  ensures !even(x+1)
{
}