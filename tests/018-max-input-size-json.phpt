--TEST--
EPV: epv.max_input_size rejects oversized JSON request bodies
--SKIPIF--
<?php if (!extension_loaded('epv')) die('skip epv not loaded'); ?>
--INI--
epv.max_input_size=10
display_errors=0
log_errors=0
--CGI--
--ENV--
CONTENT_TYPE=application/json
--PUT--
{"name":"John","age":30}
--FILE--
<?php
// Body is 24 bytes but limit is 10; body should be silently rejected
var_dump($_PUT);
?>
--EXPECT--
array(0) {
}
