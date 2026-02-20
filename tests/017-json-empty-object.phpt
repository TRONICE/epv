--TEST--
EPV: PUT request with empty JSON object body results in empty array
--SKIPIF--
<?php
if (!extension_loaded('epv')) die('skip epv not loaded');
if (!extension_loaded('json')) die('skip json not loaded');
?>
--CGI--
--ENV--
CONTENT_TYPE=application/json
--PUT--
{}
--FILE--
<?php
var_dump($_PUT);
?>
--EXPECT--
array(0) {
}
