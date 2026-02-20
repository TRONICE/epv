--TEST--
EPV: PUT request with multipart/form-data body
--SKIPIF--
<?php if (!extension_loaded('epv')) die('skip epv not loaded'); ?>
--CGI--
--ENV--
CONTENT_TYPE=multipart/form-data; boundary=testboundary
--PUT--
--testboundary
Content-Disposition: form-data; name="field1"

value1
--testboundary--
--FILE--
<?php
var_dump($_PUT);
?>
--EXPECT--
array(1) {
  ["field1"]=>
  string(6) "value1"
}
