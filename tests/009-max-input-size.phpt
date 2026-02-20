--TEST--
EPV: epv.max_input_size rejects oversized request bodies
--SKIPIF--
<?php if (!extension_loaded('epv')) die('skip epv not loaded'); ?>
--INI--
epv.max_input_size=5
display_errors=0
log_errors=0
--CGI--
--ENV--
CONTENT_TYPE=application/x-www-form-urlencoded
--PUT--
name=John
--FILE--
<?php
// Body is 9 bytes but limit is 5; body should be silently rejected
var_dump($_PUT);
?>
--EXPECT--
array(0) {
}
