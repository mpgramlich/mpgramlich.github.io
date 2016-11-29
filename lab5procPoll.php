<html><head>
		<meta http-equiv="Content-Type" content="text/html; charset=windows-1252">
		<title>Matt Gramlich Lab 4: PHP</title>
		<link rel="stylesheet" type="text/css" href="lab5stylesheet.css">
		<style>
            h1 {font-size:25px;}
        </style>
		<script>
			
			function changePage()
			{
			    navDrop = document.getElementById("navDropDown");
			    window.location.href = navDrop.value;
			}
			
		</script>
	</head>
  
	<body>
	<?php 
	    $dayVote=-1;
        if(empty($_POST["day"]))
        {
            echo("No input, ignoring");
            header("Location: lab5results.php"); /*http://stackoverflow.com/questions/2112373/php-page-redirect*/
            exit();
        }
        $dayVote=$_POST["day"][0];
        
        $fileContent = unserialize(file_get_contents("voteCount.txt"));
        if(!is_array($fileContent))
        {
            echo("Could not load file, zeroed array <br>");
            $dayArr = array(0,0,0,0,0);
            $fileContent = array("dayVoteArr"=>$dayArr);
        }
        
        $file = fopen("voteCount.txt","w") or die("Oh No! Cannot open file!");
        
        $fileContent["dayVoteArr"][$dayVote] = $fileContent["dayVoteArr"][$dayVote] + 1;
        
        for($i=0; $i<count($fileContent["dayVoteArr"]); $i++)
        {
            $val = $fileContent["dayVoteArr"][$i];
            echo("day{$i}: {$val} <br>");
        }
        
        fwrite($file, serialize($fileContent));
        fclose($file);
        echo("wrote array");

        header("Location: lab5results.php"); /*http://stackoverflow.com/questions/2112373/php-page-redirect*/
        exit();
    ?>

		<hr>
        <p>
            <textbody> created by: Matthew Gramlich, 9/23/2016</textbody>
        </p>
	

</body></html>
