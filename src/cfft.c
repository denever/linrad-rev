/*	
	Complex forward split radix-2 DIT FFT:	
	complex input in {x, y}, full cycle
	cos/sin tables in {cs, ss}, length = 
	n = 2**m. Requires cbitrev(x, y, n) 
	prior to the SR-2 transform.
*/

void	cfffts2(float *x, float *y, long n, long m, float *cs, float *ss)

{

 	long	i, j, k, q, ii, i0, i1, i2, i3, n1, n2, n4, is, id;
	float	cc1, ss1, cc3, ss3, r1, r2, r3, s1, s2;

/*	Length two butterflies	*/
 
	is = 0;
	id = 1;
	n1 = n - 1;
  	
	do
	{
		id <<= 2;

		for (i0 = is; i0 < n; i0 += id)
		{
	 		i1 = i0 + 1;

			r1 = x[i0];
			x[i0] = r1 + x[i1];
			x[i1] = r1 - x[i1];

			r1 = y[i0];
			y[i0] = r1 + y[i1];
			y[i1] = r1 - y[i1]; 
		}

		is = (id - 1) << 1;

	} while (is < n1);

/*	L shaped butterflies	*/

	n2 = 2;
	
	for (k = 2; k <= m; k++)
	{
		n4 = n2 >> 1;
	 	n2 <<= 1;
	
		is = 0;
		id = n2 << 1;

	/*	Unity twiddle factor loops	*/

		do
		{
			for (i0 = is; i0 < n1; i0 += id)
			{
				i1 = i0 + n4;
				i2 = i1 + n4;
				i3 = i2 + n4;

				r3 = x[i2] + x[i3];
				r2 = x[i2] - x[i3];
				r1 = y[i2] + y[i3];
				s2 = y[i2] - y[i3];

				x[i2] = x[i0] - r3;
				x[i0] += r3;
				x[i3] = x[i1] - s2;
				x[i1] += s2;

				y[i2] = y[i0] - r1;
				y[i0] += r1;
				y[i3] = y[i1] + r2;
				y[i1] += -r2;	
			} 
			
			is = (id << 1) - n2;
			id <<= 2;

		} while (is < n1);

		q = ii = n / n2;

	/*	Non-trivial twiddle factor loops	*/

		for (j = 1; j < n4; j++, q += ii)
		{
			cc1 = cs[q];
			ss1 = ss[q];
			i3 = q * 3;
			cc3 = cs[i3];
			ss3 = ss[i3];
			
			is = j;
			id = n2 << 1;

			do
			{
				for (i0 = is; i0 < n1; i0 += id)
				{
					i1 = i0 + n4;
					i2 = i1 + n4;
					i3 = i2 + n4;

					r1 = x[i2] * cc1 + y[i2] * ss1;
					s1 = y[i2] * cc1 - x[i2] * ss1;
					r2 = x[i3] * cc3 + y[i3] * ss3;
					s2 = y[i3] * cc3 - x[i3] * ss3;

					r3 = r1 + r2;
					r2 = r1 - r2;
					r1 = s1 + s2;
					s2 = s1 - s2;

					x[i2] = x[i0] - r3;
					x[i0] += r3;
					x[i3] = x[i1] - s2;
					x[i1] += s2;

					y[i2] = y[i0] - r1;
					y[i0] += r1;
					y[i3] = y[i1] + r2;
					y[i1] += -r2;	
				} 
			
				is = (id << 1) - n2 + j;
				id <<= 2;

			} while (is < n1);
		}
	}

}
