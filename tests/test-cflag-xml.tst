FLAGS='-X -c bbb'
STDIN='aaa,bbb,ccc\n"a1","b1","c1"\n"a2","b2","c2"\n'
STDOUT='<?xml version="1.0" encoding="UTF-8"?>\n<csv>\n  <row>\n    <bbb>b1</bbb>\n  </row>\n  <row>\n    <bbb>b2</bbb>\n  </row>\n</csv>\n'
STDERR=''
EXITVAL='0'
