--TEST--
EPV: superglobals are empty arrays when request method is not PUT/DELETE/PATCH
--SKIPIF--
<?php if (!extension_loaded('epv')) die('skip epv not loaded'); ?>
--FILE--
<?php
// In CLI mode, request_method is NULL, so no body is parsed.
// Auto-global callbacks initialize each as an empty array.
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
