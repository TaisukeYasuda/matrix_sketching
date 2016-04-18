#include <iostream>
#include "../interface/SKMatrix.hpp"
#include <armadillo>
#include <random>

using namespace arma; 
class Armadillo_Matrix: SKMatrix<Armadillo_Matrix, arma::mat>{
    private:
        mat matrix_data; 
    public:

        Armadillo_Matrix(){
            //std::cout << "constructor" << std::endl; 
            matrix_data = mat(); 
        }

        ~Armadillo_Matrix(){
        }

        Armadillo_Matrix(int r, int c){
            matrix_data = mat(r,c); 
        }

        mat data(){
            return mat(matrix_data); 
        }


        Armadillo_Matrix(mat other){
            this->matrix_data = other; 
        } 

        Armadillo_Matrix(const Armadillo_Matrix& other){
            //std::cout << "using copy operator" << std::endl; 
            auto temp = other.matrix_data; 
            this->matrix_data = mat(temp); 
        }

        Armadillo_Matrix& operator=(const Armadillo_Matrix& other){
            this->matrix_data = mat(other.matrix_data); 
            return *this; 
        }

        Armadillo_Matrix(Armadillo_Matrix&& other){
            this->matrix_data = other.matrix_data; 

            //might not need to do this
            other.matrix_data = mat(); 
        }

        Armadillo_Matrix& operator=(Armadillo_Matrix&& other){
            this->matrix_data = other.matrix_data;
            other.matrix_data = mat(); 
            return *this; 
        }


        Armadillo_Matrix mult(Armadillo_Matrix& rhs){
            mat a = this->matrix_data * rhs.matrix_data;
            return std::move(Armadillo_Matrix(a)); 
        }

        Armadillo_Matrix rand_n(int row, int col, int mean, int std){
            std::cout << "filling with gaussian noise" << std::endl; 
            mat a(row, col); 
            a.randn(); 
            a = a*std + mean; 
            this->matrix_data = a; 
            return *this; 
        }
        Armadillo_Matrix elem_div(const double b){
           mat a = data(); 
           if (b != 0.0){
               a = a/b;
           }
           else{
               std::cout << "cannot divide by 0" << std::endl; 
           }
           return std::move(Armadillo_Matrix(a)); 
        }

        std::vector<int> flip_signs(){
            auto indices = std::vector<int>(); 
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1, 2);
            int n_cols = this->matrix_data.n_cols; 
            for(int i=0; i<n_cols; i++){
                int num = dis(gen); 
                if (num == 2){
                    indices.push_back(-1); 
                } 
                else{
                    indices.push_back(1); 
                }
            }
            return std::move(indices); 
        }

        std::vector<int> bucket(const int num_buckets){
            auto indices = std::vector<int>(); 
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, num_buckets-1);
            int n_cols = this->matrix_data.n_cols; 
            for(int i=0; i<n_cols; i++){
                int num = dis(gen); 
                indices.push_back(num); 
            }
            return std::move(indices); 
        }
        
        Armadillo_Matrix concat(const Armadillo_Matrix& column){
            mat a = data(); 
            a.insert_cols(a.n_cols-1, column.matrix_data); 
            return std::move(Armadillo_Matrix(a)); 
        }

        Armadillo_Matrix solve_x(const Armadillo_Matrix& A, const Armadillo_Matrix& B){
                auto X = solve(A.matrix_data, B.matrix_data); 
                return std::move(Armadillo_Matrix(X)); 
        }
};

