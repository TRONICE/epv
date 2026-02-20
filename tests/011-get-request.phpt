--TEST--
EPV: GET request leaves all superglobals as empty arrays
--SKIPIF--
<?php if (!extension_loaded('epv')) die('skip epv not loaded'); ?>
--CGI--
--FILE--
<?php
var_dump($_PUT);
var_dump($_DELETE);
var_dump($_PATCH);
?>
--EXPECT--
array(0) {
}
array(0) {
}
array(0) {
}
