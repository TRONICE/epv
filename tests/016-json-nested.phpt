--TEST--
EPV: PUT request with nested JSON object and array values
--SKIPIF--
<?php
if (!extension_loaded('epv')) die('skip epv not loaded');
if (!extension_loaded('json')) die('skip json not loaded');
?>
--CGI--
--ENV--
CONTENT_TYPE=application/json
--PUT--
{"user":{"name":"John"},"tags":["php","c"]}
--FILE--
<?php
var_dump($_PUT);
?>
--EXPECT--
array(2) {
  ["user"]=>
  array(1) {
    ["name"]=>
    string(4) "John"
  }
  ["tags"]=>
  array(2) {
    [0]=>
    string(3) "php"
    [1]=>
    string(1) "c"
  }
}
