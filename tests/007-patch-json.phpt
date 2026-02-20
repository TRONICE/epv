--TEST--
EPV: PATCH request with application/json body containing nested data
--SKIPIF--
<?php
if (!extension_loaded('epv')) die('skip epv not loaded');
if (!extension_loaded('json')) die('skip json not loaded');
?>
--CGI--
--ENV--
REQUEST_METHOD=PATCH
CONTENT_TYPE=application/json
--POST_RAW--
{"id":1,"active":true}
--FILE--
<?php
var_dump($_PATCH);
?>
--EXPECT--
array(2) {
  ["id"]=>
  int(1)
  ["active"]=>
  bool(true)
}
