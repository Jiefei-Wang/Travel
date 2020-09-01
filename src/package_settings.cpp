#include <Rcpp.h>
#include <string>
using namespace Rcpp;


static std::string print_location;

// [[Rcpp::export]]
void C_set_print_location(SEXP x){
	print_location = CHAR(Rf_asChar(x));
	print_location = print_location + "/debug_output";
}
// [[Rcpp::export]]
SEXP C_get_print_location(){
	return Rf_mkString(print_location.c_str());
}

std::string get_print_location(){
    return print_location;
}



// mount point
static std::string mount_point;
// [[Rcpp::export]]
void C_set_mountpoint(SEXP path)
{
    mount_point = CHAR(Rf_asChar(path));
}
// [[Rcpp::export]]
SEXP C_get_mountpoint(){
	return Rf_mkString(get_mountpoint().c_str());
}

std::string get_mountpoint(){
    return mount_point;
}