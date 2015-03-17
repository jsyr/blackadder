FILE(REMOVE_RECURSE
  "libblackadder.pdb"
  "libblackadder.a"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/blackadder.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
