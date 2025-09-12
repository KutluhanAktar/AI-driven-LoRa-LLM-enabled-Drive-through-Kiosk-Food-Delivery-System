<!DOCTYPE html>
<html>
<head>
    <!-- Page info. -->
	<title>Order Schedule</title> 
    <!-- Site settings. -->
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">

	<!-- link to site icon -->
	<link rel="icon" type="image/png"  href="assets/img/site_icon.png">

	<!-- Add jQuery -->
	<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.7.1/jquery.min.js"></script>

	<!-- Add order schedule style file -->
	<link rel="stylesheet" type="text/css" href="assets/style/order_schedule.css"></link>
	
	<!-- Add Google fonts. -->
	<link rel="preconnect" href="https://fonts.googleapis.com">
	<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
	<link href="https://fonts.googleapis.com/css2?family=Codystar:wght@300;400&display=swap" rel="stylesheet">
	<link href="https://fonts.googleapis.com/css2?family=Comic+Relief:wght@400;700&display=swap" rel="stylesheet">
	<link href="https://fonts.googleapis.com/css2?family=Anton&display=swap" rel="stylesheet">
	<link href="https://fonts.googleapis.com/css2?family=Pacifico&display=swap" rel="stylesheet">
</head>

<body>

<section class="main_header"><h1>Requested Order Schedule</h1></section>

<div class="order_schedule">
<table>
<tr>
<th>Account Authentication Key</th>
<th>Menu Tag</th>
<th>Deal Type</th>
<th>Station 1</th>
<th>Station 2</th>
<th>Station 3</th>
<th>Station 4</th>
<th>Station 5</th>
<th>Station 6</th>
<th>Order Status</th>
<th>Updated</th>
</tr>
<tr class="pending"><td>pending</td><td>pending</td><td>pending</td><td>pending</td><td>pending</td><td>pending</td><td>pending</td><td>pending</td><td>pending</td><td>pending</td><td>pending</td></tr>
</table>
</div>

<!-- Add the required scripts. -->
<script src="assets/script/order_schedule_update.js"></script>

</body>
</html>