--TEST--
EPV: PUT request with application/json; charset=utf-8 Content-Type is parsed correctly
--SKIPIF--
<?php
if (!extension_loaded('epv')) die('skip epv not loaded');
if (!extension_loaded('json')) die('skip json not loaded');
?>
--CGI--
--ENV--
CONTENT_TYPE=application/json; charset=utf-8
--PUT--
{"name":"John"}
--FILE--
<?php
var_dump($_PUT);
?>
--EXPECT--
array(1) {
  ["name"]=>
  string(4) "John"
}
