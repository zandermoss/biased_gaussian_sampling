#include <gsl/gsl_math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

#include <iostream>
#include <vector>

#include "CovGen.h"
#include "RanGen.h"
#include "Utils.h"
#include "Projection.h"
#include "WriteOut.h"

int main()
{
	//Define sampling parameters.	
	int nsamples = 4; //Total number, including origin sample.
	double r_final=10; //Final radius.  
	double k_cutoff=10; //Cutoff in spatial frequency.
	double nu=2; //Total field value at origin.
	
	//Initialize sample radius vector.
	gsl_vector* sample_radii = gsl_vector_alloc(nsamples);	

	//Fill sample radius vector.
	linear_vector_ramp(sample_radii,r_final);
	
	//Initialize covariance matrix:
	gsl_matrix* cov_mat = gsl_matrix_alloc(nsamples,nsamples);
	
	//Fill covariance matrix using two point functions on our gaussian field.
	//These two point functions are defined by the power spectrum, P(k),
	//defined in CovGen.cc.	
	calculate_cov(cov_mat,sample_radii,k_cutoff);


	//Construct the projection matrix Pi and offset vector btwid.
	//These will project a sample vector onto the \phi=\nu constraint
	//subspace.	
	gsl_vector* btwid = gsl_vector_alloc(nsamples);	
	gsl_matrix* Pi = gsl_matrix_alloc(nsamples,nsamples);
	construct_projector(Pi, btwid, cov_mat, nu);

	
	//Construct the covariance factor matrix A using a cholesky
	//decomposition on the covariance matrix cov_mat. This will
	//generate samples with covariance cov_mat from centered 
	//unit normal vectors.
	gsl_matrix* A=gsl_matrix_alloc(nsamples,nsamples);
	cholesky_decomp(cov_mat,A);

	//Precomputing the product (Pi*A) of projection and covariance-conversion.
    gsl_matrix* M = gsl_matrix_alloc(nsamples,nsamples);
	gsl_matrix_set_zero(M);
    gsl_blas_dgemm(CblasNoTrans,CblasNoTrans,1.0,Pi,A,0.0,M);

	//Generate a vector, z, from centered unit normal univariate distributions.
	gsl_vector* z = gsl_vector_alloc(nsamples);
	gen_normal_vector(z);

	//Transform the centered unit normal vector z into a vector y from a 
	//centered multivariate normal distribution with covariance cov_mat.
	//Project this vector onto the constraint subspace defined by A and btwid.
	gsl_vector* y = gsl_vector_alloc(nsamples);
	gsl_vector_set_zero(y);
	gsl_blas_dgemv(CblasNoTrans,1.0,M,z,0.0,y);
	gsl_vector_add(y,btwid);

	//Write out the radii and sampled field values.
	std::string filename = "fields.txt";
	std::vector<gsl_vector*> vecs = {sample_radii,y};
	write_gslvecs(vecs,filename);

	//Free memory allocated to gsl_matrix and gsl_vector structures.
	gsl_vector_free(sample_radii);
	gsl_vector_free(btwid);
	gsl_vector_free(z);
	gsl_vector_free(y);

	gsl_matrix_free(cov_mat);
	gsl_matrix_free(Pi);
	gsl_matrix_free(A);
	gsl_matrix_free(M);

	return 0;
}
