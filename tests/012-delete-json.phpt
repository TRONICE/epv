--TEST--
EPV: DELETE request with application/json body
--SKIPIF--
<?php
if (!extension_loaded('epv')) die('skip epv not loaded');
if (!extension_loaded('json')) die('skip json not loaded');
?>
--CGI--
--ENV--
REQUEST_METHOD=DELETE
CONTENT_TYPE=application/json
--POST_RAW--
{"id":42}
--FILE--
<?php
var_dump($_DELETE);
?>
--EXPECT--
array(1) {
  ["id"]=>
  int(42)
}
