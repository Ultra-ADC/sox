/* libSoX effect: Get envelope from sound. Currently it uses a rather naive algorithm that takes a max sample in a window of X samples.    (c) 2011 robs@users.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "sox_i.h"

typedef struct {
	unsigned sample_counter; // Counts number of samples
	sox_sample_t max_value; // Saves the maximum value
	unsigned window_size; // Window to look for max in
} priv_t;

static int create(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *)effp->priv;
  p->window_size = 100;
  p->max_value = 0;
  --argc, ++argv;
  do {NUMERIC_PARAMETER(window_size, 1, 10000)} while (0);
  return argc? lsx_usage(effp) : SOX_SUCCESS;
}

static int start(sox_effect_t * effp)
{
  //priv_t * p = (priv_t *) effp->priv;
  effp->out_signal.rate = effp->in_signal.rate  ;/// p->window_size;
  return SOX_SUCCESS;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;

  size_t ilen = *isamp;
  size_t olen = *osamp;
  size_t len = 0;
  sox_sample_t tmp;
      
  len = min(*isamp, *osamp);

  for(int i = 0; i < len; i++){
    p->sample_counter++;

    tmp = abs(*ibuf++);

    ilen--;
    if(p->max_value < tmp){
    	p->max_value = tmp;
    }

    if(p->sample_counter >= p->window_size){
	*obuf++ = p->max_value;
        olen--;
	p->sample_counter = 0;
	p->max_value=0;
    }
  }

  *osamp -= olen;
  *isamp -= ilen;

  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_maxenvelope_effect_fn(void)
{
  static sox_effect_handler_t handler = {"maxenvelope", "[window_size (10000)]",
    SOX_EFF_RATE | SOX_EFF_MODIFY, create, start, flow, NULL, NULL, NULL, sizeof(priv_t)};
  return &handler;
}
