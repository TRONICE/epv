--TEST--
EPV: PUT request without Content-Type results in empty array
--SKIPIF--
<?php if (!extension_loaded('epv')) die('skip epv not loaded'); ?>
--CGI--
--PUT--
name=John
--FILE--
<?php
// No CONTENT_TYPE set; EPV cannot determine how to parse the body
var_dump($_PUT);
?>
--EXPECT--
array(0) {
}
