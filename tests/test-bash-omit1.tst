FLAGS='-bi'
STDIN='aaa,PATH,ccc\n"a1","b1","c1"\n"a2","b2","c2"\n'
STDOUT=$'aaa=\'a1\'; ccc=\'c1\';\naaa=\'a2\'; ccc=\'c2\';\n'
STDERR=''
EXITVAL='0'