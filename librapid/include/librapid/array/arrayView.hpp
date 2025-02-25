#ifndef LIBRAPID_ARRAY_ARRAY_VIEW_HPP
#define LIBRAPID_ARRAY_ARRAY_VIEW_HPP

namespace librapid {
	namespace typetraits {
		template<typename T>
		struct TypeInfo<array::ArrayView<T>> {
			static constexpr detail::LibRapidType type = detail::LibRapidType::ArrayView;
			using Scalar							   = typename TypeInfo<std::decay_t<T>>::Scalar;
			using Device							   = typename TypeInfo<std::decay_t<T>>::Device;
			static constexpr bool allowVectorisation   = false;
		};
	} // namespace typetraits

	namespace array {
		template<typename T>
		class ArrayView {
		public:
			using ArrayType		 = T;
			using BaseType		 = typename std::decay_t<T>;
			using Scalar		 = typename typetraits::TypeInfo<BaseType>::Scalar;
			using Reference		 = BaseType &;
			using ConstReference = const BaseType &;
			using StrideType	 = typename BaseType::StrideType;
			using ShapeType		 = typename BaseType::ShapeType;
			using Device		 = typename typetraits::TypeInfo<BaseType>::Device;

			/// Default constructor should never be used
			ArrayView() = delete;

			/// Copy an ArrayView object (not const)
			/// \param array The array to copy
			explicit ArrayView(T &array);

			/// Copy an ArrayView object (const)
			/// \param other The array to copy
			ArrayView(const ArrayView &other) = default;

			/// Constructs an ArrayView from a temporary instance
			/// \param other The ArrayView to move
			ArrayView(ArrayView &&other) = default;

			/// Assigns another ArrayView object to this ArrayView.
			/// \param other The ArrayView to assign.
			/// \return A reference to this
			ArrayView &operator=(const ArrayView &other) = default;

			/// Assigns a temporary ArrayView to this ArrayView.
			/// \param other The ArrayView to move.
			/// \return A reference to this ArrayView.
			ArrayView &operator=(ArrayView &&other) noexcept = default;

			/// Assign a scalar value to this ArrayView. This function should only be used to
			/// assign to a zero-dimensional "scalar" ArrayView, and will throw an error if used
			/// incorrectly.
			/// \param scalar The scalar value to assign
			/// \return A reference to this
			ArrayView &operator=(const Scalar &scalar);

			/// Access a sub-array of this ArrayView.
			/// \param index The index of the sub-array.
			/// \return An ArrayView from this
			ArrayView<ArrayType> operator[](int64_t index) const;

			/// Since even scalars are represented as an ArrayView object, it can be difficult to
			/// operate on them directly. This allows you to extract the scalar value stored by a
			/// zero-dimensional ArrayView object
			/// \tparam CAST Type to cast to
			/// \return The scalar represented by the ArrayView object
			template<typename CAST = Scalar>
			LIBRAPID_NODISCARD LIBRAPID_ALWAYS_INLINE CAST get() const;

			/// Same functionality as "get", except slightly less robust for user-defined types.
			/// \tparam CAST Type to cast to
			/// \return The scalar represented by the ArrayView object
			template<typename CAST>
			LIBRAPID_NODISCARD LIBRAPID_ALWAYS_INLINE operator CAST() const;

			/// Access the underlying shape of this ArrayView
			/// \return Shape object
			LIBRAPID_NODISCARD LIBRAPID_ALWAYS_INLINE ShapeType shape() const;

			/// Access the stride of this ArrayView
			/// \return Stride object
			LIBRAPID_NODISCARD LIBRAPID_ALWAYS_INLINE StrideType stride() const;

			/// Access the offset of this ArrayView. This is the offset, in elements, from the
			/// referenced Array's first element.
			/// \return Offset
			LIBRAPID_NODISCARD LIBRAPID_ALWAYS_INLINE int64_t offset() const;

			/// Set the Shape of this ArrayView to something else. Intended for internal use only.
			/// \param shape The new shape of this ArrayView
			void setShape(const ShapeType &shape);

			/// Set the Stride of this ArrayView to something else. Intended for internal use only.
			/// \param stride The new stride of this ArrayView
			void setStride(const StrideType &stride);

			/// Set the offset of this ArrayView object. Intended for internal use only.
			/// \param offset The new offset of this ArrayView
			void setOffset(const int64_t &offset);

			/// Returns the number of dimensions of this ArrayView
			/// \return Number of dimensions
			LIBRAPID_NODISCARD LIBRAPID_ALWAYS_INLINE int64_t ndim() const;

			/// Return the Scalar at a given index in this ArrayView. This is intended for use
			/// internally, but can be used externally too.
			/// \param index The index of the Scalar to return
			/// \return Scalar at the given index
			LIBRAPID_NODISCARD LIBRAPID_ALWAYS_INLINE auto scalar(int64_t index) const;

			/// Evaluate the contents of this ArrayView object and return an Array instance from
			/// it. Depending on your use case, this may result in more performant code, but the new
			/// Array will not reference the original data in the ArrayView.
			/// \return A new Array instance
			LIBRAPID_NODISCARD LIBRAPID_ALWAYS_INLINE ArrayType eval() const;

			/// Cast an ArrayView to a std::string, aligning items down the columns. A format
			/// string can also be specified, which will be used to format the items to strings
			/// \param format The format string
			/// \return A std::string representation of this ArrayView
			LIBRAPID_NODISCARD std::string str(const std::string &format = "{}") const;

		private:
			ArrayType &m_ref;
			ShapeType m_shape;
			StrideType m_stride;
			int64_t m_offset = 0;
		};

		template<typename T>
		ArrayView<T>::ArrayView(ArrayType &array) :
				m_ref(array), m_shape(array.shape()), m_stride(array.shape()) {}

		template<typename T>
		ArrayView<T> &ArrayView<T>::operator=(const Scalar &scalar) {
			LIBRAPID_ASSERT(m_shape.ndim() == 0, "Cannot assign to a non-scalar ArrayView.");
			m_ref.storage()[m_offset] = static_cast<Scalar>(scalar);
			return *this;
		}

		template<typename T>
		auto ArrayView<T>::operator[](int64_t index) const -> ArrayView<ArrayType> {
			LIBRAPID_ASSERT(
			  index >= 0 && index < static_cast<int64_t>(m_shape[0]),
			  "Index {} out of bounds in ArrayContainer::operator[] with leading dimension={}",
			  index,
			  m_shape[0]);
			ArrayView<ArrayType> view(m_ref);
			const auto stride = Stride(m_shape);
			view.setShape(m_shape.subshape(1, ndim()));
			if (ndim() == 1)
				view.setStride(Stride({1}));
			else
				view.setStride(stride.subshape(1, ndim()));
			view.setOffset(m_offset + index * stride[0]);
			return view;
		}

		template<typename T>
		template<typename CAST>
		CAST ArrayView<T>::get() const {
			LIBRAPID_ASSERT(m_shape.ndim() == 0,
							"Can only cast a scalar ArrayView to a salar object");
			return scalar(0);
		}

		template<typename T>
		template<typename CAST>
		ArrayView<T>::operator CAST() const {
			return get();
		}

		template<typename T>
		auto ArrayView<T>::shape() const -> ShapeType {
			return m_shape;
		}

		template<typename T>
		auto ArrayView<T>::stride() const -> StrideType {
			return m_stride;
		}

		template<typename T>
		auto ArrayView<T>::offset() const -> int64_t {
			return m_offset;
		}

		template<typename T>
		void ArrayView<T>::setShape(const ShapeType &shape) {
			m_shape = shape;
		}

		template<typename T>
		void ArrayView<T>::setStride(const StrideType &stride) {
			m_stride = stride;
		}

		template<typename T>
		void ArrayView<T>::setOffset(const int64_t &offset) {
			m_offset = offset;
		}

		template<typename T>
		auto ArrayView<T>::ndim() const -> int64_t {
			return m_shape.ndim();
		}

		template<typename T>
		auto ArrayView<T>::scalar(int64_t index) const -> auto {
			if (ndim() == 0) return m_ref.scalar(m_offset);

			ShapeType tmp	= ShapeType::zeros(ndim());
			tmp[ndim() - 1] = index % m_shape[ndim() - 1];
			for (int64_t i = ndim() - 2; i >= 0; --i) {
				index /= m_shape[i + 1];
				tmp[i] = index % m_shape[i];
			}
			int64_t offset = 0;
			for (int64_t i = 0; i < ndim(); ++i) { offset += tmp[i] * m_stride[i]; }
			return m_ref.scalar(m_offset + offset);
		}

		template<typename T>
		auto ArrayView<T>::eval() const -> ArrayType {
			ArrayType res(m_shape);
			ShapeType coord = ShapeType::zeros(m_shape.ndim());
			int64_t d = 0, p = 0;
			int64_t idim = 0, adim = 0;
			const int64_t ndim = m_shape.ndim();

			do {
				res.storage()[d++] = m_ref.scalar(p + m_offset);

				for (idim = 0; idim < ndim; ++idim) {
					adim = ndim - idim - 1;
					if (++coord[adim] == m_shape[adim]) {
						coord[adim] = 0;
						p			= p - (m_shape[adim] - 1) * m_stride[adim];
					} else {
						p = p + m_stride[adim];
						break;
					}
				}
			} while (idim < ndim);

			return res;
		}
	} // namespace array

} // namespace librapid

// Support FMT printing
#ifdef FMT_API
LIBRAPID_SIMPLE_IO_IMPL(typename T, librapid::array::ArrayView<T>)
#endif // FMT_API

#endif // LIBRAPID_ARRAY_ARRAY_VIEW_HPP