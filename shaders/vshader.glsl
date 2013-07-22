/* This file is part of PlasmaView.
 *
 * PlasmaView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PlasmaView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Gneral Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PlasmaView.  If not, see <http://www.gnu.org/licenses/>.
 */

precision mediump float;

uniform mat4 u_projection;
uniform mat4 u_view;

attribute vec4 a_position;
attribute vec4 a_color;
attribute vec3 a_uvw0;

varying vec4 v_color;
varying vec3 v_texcoord0;

void main()
{
    gl_Position = u_projection * u_view * a_position;

    // Pass these along to the fragment shader
    v_color = a_color;
    v_texcoord0 = a_uvw0;
}
