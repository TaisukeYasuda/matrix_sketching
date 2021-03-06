#ifndef __BOOST_H__
#define __BOOST_H__

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/operation.hpp>
#include <boost/numeric/ublas/storage.hpp>
#include "SKMatrix.hpp"
#include <boost/numeric/bindings/traits/ublas_matrix.hpp>
#include <boost/numeric/bindings/traits/ublas_vector.hpp>
#include <boost/numeric/bindings/lapack/gesvd.hpp>

namespace bnu = boost::numeric::ublas;
namespace lap = boost::numeric::bindings::lapack;

namespace sketchy {
    class boost: public SKMatrix<boost, bnu::matrix<float> >{
        public:
            boost(){
                this->matrix_data = bnu::matrix<float>();
            }

            boost(const int row, const int col){
                if(row < 0 || col < 0) {
                    throw std::invalid_argument("Column and row lengths must be non-negative integers");
                } else {
                    this->matrix_data = bnu::matrix<float>(row, col, 0);
                }
            }

            boost(const bnu::matrix<float>& mat){
                this->matrix_data = mat;
            }

            ~boost() = default;

            void eye(const int n) {
                matrix_data = bnu::identity_matrix<float>(n);
            }

            boost& operator=(const boost& rhs){
                matrix_data = rhs.data();
                return *this;
            }

            boost& operator=(const bnu::matrix<float>& rhs){
                matrix_data = bnu::matrix<float>(rhs);
                return *this;
            }

            bnu::matrix<float> data(void) const { return bnu::matrix<float>(matrix_data); }

            friend std::ostream& operator<<(std::ostream&os, const boost& mat);

            void clear() { matrix_data.clear(); }
            int size() const { return matrix_data.size1() * matrix_data.size2(); }

            int num_rows(void) const { return matrix_data.size1(); }
            int num_cols(void) const { return matrix_data.size2(); }

            boost mult(const boost& rhs) const;

            boost rand_n(const int row, const int col);
            boost elem_div(const float a) const;


            /* Regression */
            boost concat(const boost& mat) const;
            boost solve_x(const boost& B) const;

            boost get_cols(const int start, const int end) const;
            boost get_col(const int col_n) const;

            void transpose() {
                matrix_data = bnu::trans(matrix_data);
            };

            boost subtract(const boost& rhs) const;

            float accumulate() const {
                bnu::matrix<float> temp(data());
                return bnu::sum(bnu::prod(bnu::scalar_vector<float>(temp.size1()), temp));
            }

            /* floatODO: K-SVD */
            void qr_decompose(boost& Q, boost& R) const;

            void svd(boost& U, boost& S, boost& V, const int k) const;
    };

    std::ostream& operator<<(std::ostream&os, const boost& mat){
        std::ostringstream out;
        out << mat.data();
        os << out.str();
        return os;
    }

    boost boost::mult(const boost& rhs) const {
        if (rhs.num_rows() != num_cols()) {
            throw std::invalid_argument("Column of left matrix does not match row of right matrix");
        } else {
            bnu::matrix<float> prod(num_rows(), rhs.num_cols());
            bnu::noalias(prod) = bnu::prod(this->data(), rhs.data());
            return std::move(boost(prod));
        }
    }

    boost boost::rand_n(const int row, const int col) {
        if (row < 0 || col < 0) {
            throw std::invalid_argument("Column and row lengths must be non-negative integers");
        } else {
            std::default_random_engine generator;
            std::normal_distribution<float> distribution(0, 1);

            bnu::matrix<float> rand_matrix(row, col);
            for (unsigned i = 0; i < rand_matrix.size1 (); ++ i){
                for (unsigned j = 0; j < rand_matrix.size2 (); ++ j){
                    rand_matrix(i, j) = distribution(generator);
                }
            }
            matrix_data = rand_matrix;
            return *this;
        }
    }

    boost boost::elem_div(const float a) const {
        if(a == 0) {
            throw std::overflow_error("Cannot divide by 0" );
        } else{
            bnu::matrix<float> result = this->data()/a;
            return std::move(boost(result));
        }
    }

    boost boost::concat(const boost& mat) const {
        if (mat.num_rows() != this->num_rows()) {
            throw std::invalid_argument("Number of rows do not match");
        } else {
            int rows = this->num_rows();
            int cols = this->num_cols();
            int new_cols = cols + mat.num_cols();
            bnu::matrix<float> c(rows, new_cols);

            bnu::project(c, bnu::range(0, rows), bnu::range(0, cols)) = this->data();
            bnu::project(c, bnu::range(0, rows), bnu::range(cols, new_cols)) = mat.data();

            return std::move(boost(c));
        }
    }

    boost boost::solve_x(const boost& B) const {
        if(this->num_rows() != this->num_cols())
            throw std::invalid_argument("A must be square in Ax = B");
        if(this->num_cols() != B.num_rows())
            throw std::invalid_argument("B.rows must equal A.cols in Ax = B");
        if(B.num_cols() != 1)
            throw std::invalid_argument("B must be n x 1 vector in Ax = B");

        bnu::matrix<float> A(this->data());
        bnu::matrix<float> y = bnu::trans(B.data());

        bnu::vector<float> b(B.num_rows());
        std::copy(y.begin1(), y.end1(), b.begin());

        bnu::permutation_matrix<> piv(b.size());
        bnu::lu_factorize(A, piv);
        bnu::lu_substitute(A, piv, b);

        bnu::matrix<float> x(this->num_rows(), 1);
        std::copy(b.begin(), b.end(), x.begin1());
        return std::move(boost(x));
    }

    void boost::qr_decompose(boost& Q, boost& R) const {
        float mag;
        float alpha;

        bnu::matrix<float> u(this->num_rows(), 1);
        bnu::matrix<float> v(this->num_rows(), 1);

        bnu::identity_matrix<float> I(this->num_rows());

        bnu::matrix<float> q = bnu::identity_matrix<float>(this->num_rows());
        bnu::matrix<float> r(data());

        for (int i = 0; i < num_rows(); i++) {
            bnu::matrix<float> p(num_rows(), num_rows());

            u.clear();
            v.clear();

            mag = 0.0;

            for (int j = i; j < num_cols(); j++) {
                u(j,0) = r(j, i);
                mag += u(j,0) * u(j,0);
            }

            mag = std::sqrt(mag);

            alpha = -1 * mag;

            mag = 0.0;

            for (int j = i; j < num_cols(); j++) {
                v(j,0) = j == i ? u(j,0) + alpha : u(j,0);
                mag += v(j,0) * v(j,0);
            }

            mag = std::sqrt(mag); // norm

            if  (mag < 0.0000000001) continue;

            v /= mag;

            p = I - (bnu::prod(v, bnu::trans(v))) * 2.0;
            q = bnu::prod(q, p);
            r = bnu::prod(p, r);
        }

        Q = boost(q);
        R = boost(r);
    }

    boost boost::subtract(const boost& rhs) const {\
        if(rhs.num_rows() != this->num_rows()){
            throw std::invalid_argument("Number of rows do not match");
        } else {
            return std::move(boost(this->data() - rhs.data()));
        }
    }

    boost boost::get_col(const int col_n) const {
        if(col_n < 0 || col_n >= num_cols()) {
            throw std::range_error("Column index out of bound");
        } else {
            bnu::matrix<float> column = bnu::subrange(data(), 0, num_rows(), col_n, col_n+1);
            return std::move(boost(column));
        }
    };

    boost boost::get_cols(const int start, const int end) const {
        if (start < 0 || end > num_cols()) {
            throw std::range_error("Column index out of bound");
            throw;
        } else if (start > end){
            throw std::invalid_argument("Start column greater than end column");
        } else {
            bnu::matrix<float> columns = bnu::subrange(data(), 0, num_rows(), start, end);
            return boost(columns);
        }
    }

    void boost::svd(boost& U, boost& S, boost& V, const int k) const {
        bnu::matrix<float> a(matrix_data);
        bnu::matrix<float> u(this->num_rows(), this->num_rows());
        bnu::vector<float> s(this->num_cols());
        bnu::matrix<float> v(this->num_cols(), this->num_cols());

        lap::gesvd('A', 'A', a, s, u, v);
        U = u;
        bnu::matrix<float> sS(k, 1);

        std::copy(s.begin(), s.end(), sS.begin1());

        S = sS;
        V = v;
    };
}

#endif