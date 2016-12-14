var app=angular.module('real-time', []);

app.controller('update_control', function($scope,$http,$interval){
	update_data();
	$interval(function(){
		update_data();
	},3000);

	function update_data(){
		$http.get('load.txt').success(function(data){
			$scope.update_data = data;
		});
	;}

});